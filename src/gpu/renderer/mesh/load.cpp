/*
Copyright (C) 2017-2023 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "load.h"

#include "buffers/material.h"
#include "shaders/vertex_points.h"
#include "shaders/vertex_triangles.h"

#include <src/com/chrono.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/image/image.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
constexpr float MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL = 0.7;

constexpr Vector2f NULL_TEXTURE_COORDINATES = Vector2f(-1e10);

// clang-format off
constexpr std::array COLOR_IMAGE_FORMATS
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

std::string time_string(const double time)
{
        return to_string_fixed(1000.0 * time, 5) + " ms";
}

class Vertex final
{
        Vector3f p_;
        Vector3f n_;
        Vector2f t_;
        std::size_t hash_;

public:
        void set(const Vector3f& p, const Vector3f& n, const Vector2f& t)
        {
                p_ = p;
                n_ = n;
                t_ = t;
                hash_ = compute_hash(p[0], p[1], p[2], n[0], n[1], n[2], t[0], t[1]);
        }

        [[nodiscard]] const Vector3f& p() const
        {
                return p_;
        }

        [[nodiscard]] const Vector3f& n() const
        {
                return n_;
        }

        [[nodiscard]] const Vector2f& t() const
        {
                return t_;
        }

        [[nodiscard]] const std::size_t& hash() const
        {
                return hash_;
        }
};

class MapVertex final
{
        const Vertex* data_;

        [[nodiscard]] std::size_t hash() const noexcept
        {
                return data_->hash();
        }

public:
        explicit MapVertex(const Vertex* const v) noexcept
                : data_(v)
        {
        }

        [[nodiscard]] bool operator==(const MapVertex& v) const noexcept
        {
                return data_->p() == v.data_->p() && data_->n() == v.data_->n() && data_->t() == v.data_->t();
        }

        struct Hash final
        {
                [[nodiscard]] std::size_t operator()(const MapVertex& v) const
                {
                        return v.hash();
                }
        };
};

std::array<Vector3f, 3> face_vertices(const model::mesh::Mesh<3>& mesh, const model::mesh::Mesh<3>::Facet& mesh_facet)
{
        std::array<Vector3f, 3> res;
        for (int i = 0; i < 3; ++i)
        {
                res[i] = mesh.vertices[mesh_facet.vertices[i]];
        }
        return res;
}

std::array<Vector3f, 3> face_normals(const Vector3f& geometric_normal)
{
        std::array<Vector3f, 3> res;
        for (int i = 0; i < 3; ++i)
        {
                res[i] = geometric_normal;
        }
        return res;
}

std::array<Vector3f, 3> face_normals(
        const model::mesh::Mesh<3>& mesh,
        const model::mesh::Mesh<3>::Facet& mesh_facet,
        const std::array<Vector3f, 3>& vertices)
{
        const Vector3f geometric_normal = cross(vertices[1] - vertices[0], vertices[2] - vertices[0]).normalized();
        if (!is_finite(geometric_normal))
        {
                error("Face unit orthogonal vector is not finite for the face with vertices (" + to_string(vertices[0])
                      + ", " + to_string(vertices[1]) + ", " + to_string(vertices[2]) + ")");
        }

        if (!mesh_facet.has_normal)
        {
                return face_normals(geometric_normal);
        }

        const bool use_mesh_normals = [&]
        {
                static_assert(MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL > 0);
                for (int i = 0; i < 3; ++i)
                {
                        const auto d = dot(mesh.normals[mesh_facet.normals[i]], geometric_normal);
                        if (!(std::isfinite(d) && std::abs(d) >= MIN_COSINE_VERTEX_NORMAL_FACET_NORMAL))
                        {
                                return false;
                        }
                }
                return true;
        }();

        if (!use_mesh_normals)
        {
                return face_normals(geometric_normal);
        }

        std::array<Vector3f, 3> res;
        for (int i = 0; i < 3; ++i)
        {
                res[i] = mesh.normals[mesh_facet.normals[i]];
        }
        return res;
}

std::array<Vector2f, 3> face_texcoords(const model::mesh::Mesh<3>& mesh, const model::mesh::Mesh<3>::Facet& mesh_facet)
{
        if (mesh_facet.has_texcoord)
        {
                std::array<Vector2f, 3> res;
                for (int i = 0; i < 3; ++i)
                {
                        res[i] = mesh.texcoords[mesh_facet.texcoords[i]];
                }
                return res;
        }

        std::array<Vector2f, 3> res;
        for (int i = 0; i < 3; ++i)
        {
                res[i] = NULL_TEXTURE_COORDINATES;
        }
        return res;
}

void set_face_vertices(
        const model::mesh::Mesh<3>& mesh,
        const model::mesh::Mesh<3>::Facet& mesh_facet,
        std::array<Vertex, 3>* const face)
{
        const std::array<Vector3f, 3> v = face_vertices(mesh, mesh_facet);
        const std::array<Vector3f, 3> n = face_normals(mesh, mesh_facet, v);
        const std::array<Vector2f, 3> t = face_texcoords(mesh, mesh_facet);

        for (int i = 0; i < 3; ++i)
        {
                (*face)[i].set(v[i], n[i], t[i]);
        }
}

std::vector<std::array<Vertex, 3>> create_faces(
        const model::mesh::Mesh<3>& mesh,
        const std::vector<int>& sorted_face_indices)
{
        std::vector<std::array<Vertex, 3>> faces(sorted_face_indices.size());

        run_in_threads(
                [&](std::atomic_size_t& task)
                {
                        const std::size_t size = sorted_face_indices.size();
                        std::size_t index = 0;
                        while ((index = task++) < size)
                        {
                                set_face_vertices(mesh, mesh.facets[sorted_face_indices[index]], &faces[index]);
                        }
                },
                sorted_face_indices.size());

        return faces;
}

BufferMesh create_buffer_mesh(const std::vector<std::array<Vertex, 3>>& faces)
{
        BufferMesh mesh;

        mesh.vertices.reserve(3 * faces.size());
        mesh.indices.reserve(3 * faces.size());

        std::unordered_map<MapVertex, VertexIndexType, MapVertex::Hash> map;
        map.reserve(3 * faces.size());

        for (const std::array<Vertex, 3>& face_vertices : faces)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const auto [iter, inserted] = map.emplace(&face_vertices[i], map.size());
                        if (inserted)
                        {
                                const Vertex& vertex = face_vertices[i];
                                mesh.vertices.emplace_back(vertex.p(), vertex.n(), vertex.t());
                        }
                        mesh.indices.push_back(iter->second);
                }
        }

        ASSERT((mesh.indices.size() >= 3) && (mesh.indices.size() % 3 == 0));

        return mesh;
}

void load_mesh_to_buffers(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const BufferMesh& mesh,
        std::unique_ptr<vulkan::BufferWithMemory>* const vertex_buffer,
        std::unique_ptr<vulkan::BufferWithMemory>* const index_buffer)
{
        *vertex_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(mesh.vertices));
        (*vertex_buffer)->write(command_pool, queue, data_size(mesh.vertices), data_pointer(mesh.vertices));

        *index_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(mesh.indices));
        (*index_buffer)->write(command_pool, queue, data_size(mesh.indices), data_pointer(mesh.indices));
}

std::string mesh_info(
        const BufferMesh& mesh,
        const double create_duration,
        const double map_duration,
        const double load_duration)
{
        std::ostringstream oss;
        oss << "Mesh info" << '\n';
        oss << "  create  : " << time_string(create_duration) << '\n';
        oss << "  map     : " << time_string(map_duration) << '\n';
        oss << "  load    : " << time_string(load_duration) << '\n';
        oss << "  vertices: ";
        oss << to_string_digit_groups(mesh.vertices.size());
        oss << " (" << to_string_digit_groups(data_size(mesh.vertices)) << " bytes)" << '\n';
        oss << "  faces   : ";
        oss << to_string_digit_groups(mesh.indices.size() / 3);
        oss << " (" << to_string_digit_groups(data_size(mesh.indices)) << " bytes)";
        return oss.str();
}
}

void load_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh,
        const std::vector<int>& sorted_face_indices,
        std::unique_ptr<vulkan::BufferWithMemory>* const vertex_buffer,
        std::unique_ptr<vulkan::BufferWithMemory>* const index_buffer,
        BufferMesh* const buffer_mesh)
{
        if (mesh.facets.empty())
        {
                vertex_buffer->reset();
                index_buffer->reset();
                *buffer_mesh = {};
                return;
        }

        ASSERT(sorted_face_indices.size() == mesh.facets.size());

        //

        const Clock::time_point create_start_time = Clock::now();

        const std::vector<std::array<Vertex, 3>> faces = create_faces(mesh, sorted_face_indices);

        const double create_duration = duration_from(create_start_time);

        //

        const Clock::time_point map_start_time = Clock::now();

        *buffer_mesh = create_buffer_mesh(faces);

        const double map_duration = duration_from(map_start_time);

        //

        const Clock::time_point load_start_time = Clock::now();

        load_mesh_to_buffers(device, command_pool, queue, family_indices, *buffer_mesh, vertex_buffer, index_buffer);

        const double load_duration = duration_from(load_start_time);

        //

        LOG(mesh_info(*buffer_mesh, create_duration, map_duration, load_duration));
}

std::unique_ptr<vulkan::BottomLevelAccelerationStructure> load_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const BufferMesh& buffer_mesh)
{
        if (buffer_mesh.indices.empty())
        {
                return {};
        }

        const Clock::time_point start_time = Clock::now();

        std::vector<Vector3f> vertices;
        vertices.reserve(buffer_mesh.vertices.size());
        for (const TrianglesVertex& v : buffer_mesh.vertices)
        {
                vertices.push_back(v.position);
        }

        vulkan::BottomLevelAccelerationStructure acceleration_structure =
                vulkan::create_bottom_level_acceleration_structure(
                        device, compute_command_pool, compute_queue, family_indices, vertices, buffer_mesh.indices,
                        std::nullopt);

        const double duration = duration_from(start_time);

        LOG("Mesh acceleration structure info: " + time_string(duration));

        return std::make_unique<vulkan::BottomLevelAccelerationStructure>(std::move(acceleration_structure));
}

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        if (mesh.points.empty())
        {
                return {};
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(mesh.points.size());

        for (const model::mesh::Mesh<3>::Point& p : mesh.points)
        {
                vertices.emplace_back(mesh.vertices[p.vertex]);
        }

        auto buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));

        buffer->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        return buffer;
}

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        if (mesh.lines.empty())
        {
                return {};
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(2 * mesh.lines.size());

        for (const model::mesh::Mesh<3>::Line& line : mesh.lines)
        {
                for (const int index : line.vertices)
                {
                        vertices.emplace_back(mesh.vertices[index]);
                }
        }

        auto buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));

        buffer->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        return buffer;
}

std::vector<vulkan::ImageWithMemory> load_textures(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        const std::vector<VkFormat> formats(std::cbegin(COLOR_IMAGE_FORMATS), std::cend(COLOR_IMAGE_FORMATS));

        std::vector<vulkan::ImageWithMemory> textures;

        for (const image::Image<2>& image : mesh.images)
        {
                textures.emplace_back(
                        device, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(image.size[0], image.size[1]),
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                        command_pool, queue);
                textures.back().write(
                        command_pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        image.color_format, image.pixels);
        }

        // texture for materials without texture
        textures.emplace_back(
                device, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, vulkan::make_extent(1, 1),
                VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool, queue);

        return textures;
}

std::vector<MaterialBuffer> load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        std::vector<MaterialBuffer> buffers;
        buffers.reserve(mesh.materials.size() + 1);

        for (const model::mesh::Mesh<3>::Material& mesh_material : mesh.materials)
        {
                const Vector3f color = mesh_material.color.rgb32().clamp(0, 1);
                const bool use_texture = (mesh_material.image >= 0);
                const bool use_material = true;
                buffers.emplace_back(device, command_pool, queue, family_indices, color, use_texture, use_material);
        }

        // material for vertices without material
        constexpr Vector3f COLOR(0);
        constexpr bool USE_TEXTURE = false;
        constexpr bool USE_MATERIAL = false;
        buffers.emplace_back(device, command_pool, queue, family_indices, COLOR, USE_TEXTURE, USE_MATERIAL);

        return buffers;
}
}
