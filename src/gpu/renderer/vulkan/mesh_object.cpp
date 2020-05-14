/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "mesh_object.h"

#include "shader/buffers.h"
#include "shader/vertex_points.h"
#include "shader/vertex_triangles.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/model/mesh_utility.h>
#include <src/vulkan/buffers.h>

#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<VkFormat> COLOR_IMAGE_FORMATS =
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

// Число используется в шейдере для определения наличия текстурных координат
constexpr vec2f NO_TEXTURE_COORDINATES = vec2f(-1e10);

constexpr VkIndexType VULKAN_INDEX_TYPE = VK_INDEX_TYPE_UINT32;

using IndexType = std::conditional_t<
        VULKAN_INDEX_TYPE == VK_INDEX_TYPE_UINT32,
        uint32_t,
        std::conditional_t<VULKAN_INDEX_TYPE == VK_INDEX_TYPE_UINT16, uint16_t, void>>;

std::string time_string(double time)
{
        return to_string_fixed(1000.0 * time, 5) + " ms";
}

class Face
{
        static size_t hash(const vec3f& p, const vec3f& n, const vec2f& t)
        {
                return pack_hash(p[0], p[1], p[2], n[0], n[1], n[2], t[0], t[1]);
        }

public:
        struct Vertex
        {
                vec3f p;
                vec3f n;
                vec2f t;
        };
        struct VertexWithHash
        {
                Vertex v;
                size_t hash;
        };

        std::array<VertexWithHash, 3> vertices;

        void set(const std::array<vec3f, 3>& p, const std::array<vec3f, 3>& n, const std::array<vec2f, 3>& t)
        {
                for (int i = 0; i < 3; ++i)
                {
                        vertices[i].v.p = p[i];
                        vertices[i].v.n = n[i];
                        vertices[i].v.t = t[i];
                        vertices[i].hash = hash(p[i], n[i], t[i]);
                }
        }
};

class MapVertex
{
        const Face::VertexWithHash* m_data;

public:
        explicit MapVertex(const Face::VertexWithHash* v) noexcept : m_data(v)
        {
        }

        size_t hash() const noexcept
        {
                return m_data->hash;
        }

        bool operator==(const MapVertex& v) const noexcept
        {
                return m_data->v.p == v.m_data->v.p && m_data->v.n == v.m_data->v.n && m_data->v.t == v.m_data->v.t;
        }

        struct Hash
        {
                size_t operator()(const MapVertex& v) const
                {
                        return v.hash();
                }
        };
};

void load_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh,
        const std::vector<int>& sorted_face_indices,
        std::unique_ptr<vulkan::BufferWithMemory>* vertex_buffer,
        std::unique_ptr<vulkan::BufferWithMemory>* index_buffer,
        unsigned* vertex_count,
        unsigned* index_count)
{
        if (mesh.facets.empty())
        {
                error("No mesh facets found");
        }

        ASSERT(sorted_face_indices.size() == mesh.facets.size());

        //

        double create_time = time_in_seconds();

        std::vector<Face> faces(sorted_face_indices.size());

        const auto function = [&](std::atomic_size_t& task) {
                size_t size = sorted_face_indices.size();
                size_t index = 0;
                while ((index = task++) < size)
                {
                        std::array<vec3f, 3> p;
                        std::array<vec3f, 3> n;
                        std::array<vec2f, 3> t;

                        int face_index = sorted_face_indices[index];

                        const mesh::Mesh<3>::Facet& f = mesh.facets[face_index];

                        for (int i = 0; i < 3; ++i)
                        {
                                p[i] = mesh.vertices[f.vertices[i]];
                        }

                        if (f.has_normal)
                        {
                                for (int i = 0; i < 3; ++i)
                                {
                                        n[i] = mesh.normals[f.normals[i]];
                                }
                        }
                        else
                        {
                                vec3f geometric_normal = cross(p[1] - p[0], p[2] - p[0]).normalized();
                                if (!is_finite(geometric_normal))
                                {
                                        error("Face unit orthogonal vector is not finite for the face with vertices ("
                                              + to_string(p[0]) + ", " + to_string(p[1]) + ", " + to_string(p[2])
                                              + ")");
                                }
                                for (int i = 0; i < 3; ++i)
                                {
                                        n[i] = geometric_normal;
                                }
                        }

                        if (f.has_texcoord)
                        {
                                for (int i = 0; i < 3; ++i)
                                {
                                        t[i] = mesh.texcoords[f.texcoords[i]];
                                }
                        }
                        else
                        {
                                for (int i = 0; i < 3; ++i)
                                {
                                        t[i] = NO_TEXTURE_COORDINATES;
                                }
                        }

                        faces[index].set(p, n, t);
                }
        };

        run_in_threads(function, sorted_face_indices.size());

        create_time = time_in_seconds() - create_time;

        //

        double map_time = time_in_seconds();

        std::vector<TrianglesVertex> vertices;
        std::vector<IndexType> indices;
        std::unordered_map<MapVertex, unsigned, MapVertex::Hash> map;
        vertices.reserve(3 * mesh.facets.size());
        indices.reserve(3 * mesh.facets.size());
        map.reserve(3 * mesh.facets.size());

        for (const Face& face : faces)
        {
                for (int i = 0; i < 3; ++i)
                {
                        const auto [iter, inserted] = map.emplace(&face.vertices[i], map.size());
                        if (inserted)
                        {
                                const Face::Vertex& v = face.vertices[i].v;
                                vertices.emplace_back(v.p, v.n, v.t);
                        }
                        indices.push_back(iter->second);
                }
        }

        ASSERT((indices.size() >= 3) && (indices.size() % 3 == 0));

        map_time = time_in_seconds() - map_time;

        //

        double load_time = time_in_seconds();

        *vertex_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                data_size(vertices));
        (*vertex_buffer)->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        *index_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                data_size(indices));
        (*index_buffer)->write(command_pool, queue, data_size(indices), data_pointer(indices));

        *vertex_count = vertices.size();
        *index_count = indices.size();

        load_time = time_in_seconds() - load_time;

        //

        std::ostringstream oss;
        oss << "create = " << time_string(create_time);
        oss << ", map = " << time_string(map_time);
        oss << ", load = " << time_string(load_time);
        oss << ", vertices = " << vertices.size() << " (" << data_size(vertices) << " bytes)";
        oss << ", faces = " << indices.size() / 3 << " (" << data_size(indices) << " bytes)";
        LOG(oss.str());
}

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh)
{
        if (mesh.points.empty())
        {
                error("No mesh points found");
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(mesh.points.size());

        for (const mesh::Mesh<3>::Point& p : mesh.points)
        {
                vertices.emplace_back(mesh.vertices[p.vertex]);
        }

        std::unique_ptr<vulkan::BufferWithMemory> buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                data_size(vertices));

        buffer->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        return buffer;
}

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh)
{
        if (mesh.lines.empty())
        {
                error("No mesh lines found");
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(2 * mesh.lines.size());

        for (const mesh::Mesh<3>::Line& line : mesh.lines)
        {
                for (int index : line.vertices)
                {
                        vertices.emplace_back(mesh.vertices[index]);
                }
        }

        std::unique_ptr<vulkan::BufferWithMemory> buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                data_size(vertices));

        buffer->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        return buffer;
}

std::vector<vulkan::ImageWithMemory> load_textures(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh)
{
        constexpr bool storage = false;

        std::vector<vulkan::ImageWithMemory> textures;

        for (const typename mesh::Mesh<3>::Image& image : mesh.images)
        {
                textures.emplace_back(
                        device, command_pool, queue, family_indices, COLOR_IMAGE_FORMATS, VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_TYPE_2D, vulkan::make_extent(image.size[0], image.size[1]), VK_IMAGE_LAYOUT_UNDEFINED,
                        storage);
                textures.back().write_srgb_rgba_pixels(
                        command_pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        image.srgba_pixels);
                ASSERT(textures.back().usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT(!(textures.back().usage() & VK_IMAGE_USAGE_STORAGE_BIT));
        }

        // На одну текстуру больше для её указания, но не использования в тех материалах, где нет текстуры
        constexpr unsigned w = 1;
        constexpr unsigned h = 1;
        const std::vector<std::uint_least8_t> srgba_pixels(w * h * 4, 0);
        textures.emplace_back(
                device, command_pool, queue, family_indices, COLOR_IMAGE_FORMATS, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TYPE_2D, vulkan::make_extent(w, h), VK_IMAGE_LAYOUT_UNDEFINED, storage);
        textures.back().write_srgb_rgba_pixels(
                command_pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, srgba_pixels);
        ASSERT(textures.back().usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
        ASSERT(!(textures.back().usage() & VK_IMAGE_USAGE_STORAGE_BIT));

        return textures;
}

void load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        std::vector<MaterialBuffer>& buffers,
        std::vector<MaterialInfo>& materials)
{
        // Текстур имеется больше на одну для её использования в тех материалах, где нет текстуры
        ASSERT(textures.size() == mesh.images.size() + 1);

        const VkImageView no_texture = textures.back().image_view();

        buffers.clear();
        buffers.reserve(mesh.materials.size());
        materials.clear();
        materials.reserve(mesh.materials.size());

        for (const typename mesh::Mesh<3>::Material& mesh_material : mesh.materials)
        {
                MaterialBuffer::Material mb;
                mb.Ka = mesh_material.Ka.to_rgb_vector<float>();
                mb.Kd = mesh_material.Kd.to_rgb_vector<float>();
                mb.Ks = mesh_material.Ks.to_rgb_vector<float>();
                mb.Ns = mesh_material.Ns;
                mb.use_texture_Ka = (mesh_material.map_Ka >= 0) ? 1 : 0;
                mb.use_texture_Kd = (mesh_material.map_Kd >= 0) ? 1 : 0;
                mb.use_texture_Ks = (mesh_material.map_Ks >= 0) ? 1 : 0;
                mb.use_material = 1;
                buffers.emplace_back(device, command_pool, queue, family_indices, mb);

                ASSERT(mesh_material.map_Ka < static_cast<int>(textures.size()) - 1);
                ASSERT(mesh_material.map_Kd < static_cast<int>(textures.size()) - 1);
                ASSERT(mesh_material.map_Ks < static_cast<int>(textures.size()) - 1);

                MaterialInfo& m = materials.emplace_back();
                m.buffer = buffers.back().buffer();
                m.buffer_size = buffers.back().buffer_size();
                m.texture_Ka = (mesh_material.map_Ka >= 0) ? textures[mesh_material.map_Ka].image_view() : no_texture;
                m.texture_Kd = (mesh_material.map_Kd >= 0) ? textures[mesh_material.map_Kd].image_view() : no_texture;
                m.texture_Ks = (mesh_material.map_Ks >= 0) ? textures[mesh_material.map_Ks].image_view() : no_texture;
        }

        // На один материал больше для его указания, но не использования в вершинах, не имеющих материала
        MaterialBuffer::Material mb;
        mb.Ka = vec3f(0);
        mb.Kd = vec3f(0);
        mb.Ks = vec3f(0);
        mb.Ns = 0;
        mb.use_texture_Ka = 0;
        mb.use_texture_Kd = 0;
        mb.use_texture_Ks = 0;
        mb.use_material = 0;
        buffers.emplace_back(device, command_pool, queue, family_indices, mb);

        MaterialInfo& m = materials.emplace_back();
        m.buffer = buffers.back().buffer();
        m.buffer_size = buffers.back().buffer_size();
        m.texture_Ka = no_texture;
        m.texture_Kd = no_texture;
        m.texture_Ks = no_texture;
}
}

class MeshObject::Triangles final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        std::unique_ptr<vulkan::BufferWithMemory> m_index_buffer;
        std::vector<vulkan::ImageWithMemory> m_textures;
        std::vector<MaterialBuffer> m_material_buffers;
        std::vector<MaterialInfo> m_material_info;
        std::vector<int> m_material_vertex_offset;
        std::vector<int> m_material_vertex_count;
        unsigned m_vertex_count = 0;
        unsigned m_index_count = 0;

        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> m_material_descriptor_sets;

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

        std::function<vulkan::Descriptors(const std::vector<MaterialInfo>& materials)> m_create_descriptor_sets;

        void create_memory()
        {
                vulkan::Descriptors descriptor_sets = m_create_descriptor_sets(m_material_info);

                ASSERT(descriptor_sets.descriptor_set_count() == m_material_vertex_count.size());
                ASSERT(descriptor_sets.descriptor_set_count() == m_material_vertex_offset.size());

                m_material_descriptor_sets.erase(descriptor_sets.descriptor_set_layout());
                m_material_descriptor_sets.emplace(descriptor_sets.descriptor_set_layout(), std::move(descriptor_sets));
        }

public:
        Triangles(
                const vulkan::Device& device,
                const vulkan::CommandPool& graphics_command_pool,
                const vulkan::Queue& graphics_queue,
                const vulkan::CommandPool& /*transfer_command_pool*/,
                const vulkan::Queue& /*transfer_queue*/,
                const mesh::Mesh<3>& mesh,
                const std::function<vulkan::Descriptors(const std::vector<MaterialInfo>& materials)>&
                        create_descriptor_sets)
                : m_create_descriptor_sets(create_descriptor_sets)
        {
                ASSERT(!mesh.facets.empty());

                std::vector<int> sorted_face_indices;

                std::vector<int> material_face_offset;
                std::vector<int> material_face_count;

                sort_facets_by_material(mesh, &sorted_face_indices, &material_face_offset, &material_face_count);
                ASSERT(material_face_offset.size() == material_face_count.size());

                load_vertices(
                        device, graphics_command_pool, graphics_queue, {graphics_queue.family_index()}, mesh,
                        sorted_face_indices, &m_vertex_buffer, &m_index_buffer, &m_vertex_count, &m_index_count);
                ASSERT(m_index_count == 3 * mesh.facets.size());

                m_textures = load_textures(
                        device, graphics_command_pool, graphics_queue, {graphics_queue.family_index()}, mesh);

                load_materials(
                        device, graphics_command_pool, graphics_queue, {graphics_queue.family_index()}, mesh,
                        m_textures, m_material_buffers, m_material_info);
                ASSERT(material_face_offset.size() == m_material_info.size());

                m_material_vertex_offset.resize(material_face_count.size());
                m_material_vertex_count.resize(material_face_count.size());
                for (unsigned i = 0; i < material_face_count.size(); ++i)
                {
                        m_material_vertex_offset[i] = 3 * material_face_offset[i];
                        m_material_vertex_count[i] = 3 * material_face_count[i];
                }

                m_buffers[0] = *m_vertex_buffer;
                m_offsets[0] = 0;

                create_memory();
        }

        const vulkan::Descriptors& find_descriptor_sets(VkDescriptorSetLayout material_descriptor_set_layout) const
        {
                auto iter = m_material_descriptor_sets.find(material_descriptor_set_layout);
                if (iter == m_material_descriptor_sets.cend())
                {
                        error("Failed to find material descriptor sets for material descriptor set layout");
                }

                ASSERT(iter->second.descriptor_set_count() == m_material_vertex_count.size());
                ASSERT(iter->second.descriptor_set_count() == m_material_vertex_offset.size());

                return iter->second;
        }

        void draw_commands(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const
        {
                const vulkan::Descriptors& descriptor_sets = find_descriptor_sets(material_descriptor_set_layout);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_index_buffer, 0, VULKAN_INDEX_TYPE);

                for (unsigned i = 0; i < m_material_vertex_count.size(); ++i)
                {
                        if (m_material_vertex_count[i] <= 0)
                        {
                                continue;
                        }

                        bind_material_descriptor_set(descriptor_sets.descriptor_set(i));

                        vkCmdDrawIndexed(
                                command_buffer, m_material_vertex_count[i], 1, m_material_vertex_offset[i], 0, 0);
                }
        }

        void draw_commands_plain(VkCommandBuffer command_buffer) const
        {
                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_index_buffer, 0, VULKAN_INDEX_TYPE);

                vkCmdDrawIndexed(command_buffer, m_index_count, 1, 0, 0, 0);
        }

        void draw_commands_vertices(VkCommandBuffer command_buffer) const
        {
                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
        }
};

class MeshObject::Lines final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        unsigned m_vertex_count;

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

public:
        Lines(const vulkan::Device& device,
              const vulkan::CommandPool& graphics_command_pool,
              const vulkan::Queue& graphics_queue,
              const vulkan::CommandPool& /*transfer_command_pool*/,
              const vulkan::Queue& /*transfer_queue*/,
              const mesh::Mesh<3>& mesh)
        {
                ASSERT(!mesh.lines.empty());

                m_vertex_buffer = load_line_vertices(
                        device, graphics_command_pool, graphics_queue, {graphics_queue.family_index()}, mesh);
                m_vertex_count = 2 * mesh.lines.size();

                m_buffers[0] = *m_vertex_buffer;
                m_offsets[0] = 0;
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
        }
};

class MeshObject::Points final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        unsigned m_vertex_count;

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

public:
        Points(const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const vulkan::CommandPool& /*transfer_command_pool*/,
               const vulkan::Queue& /*transfer_queue*/,
               const mesh::Mesh<3>& mesh)
        {
                ASSERT(!mesh.points.empty());

                m_vertex_buffer = load_point_vertices(
                        device, graphics_command_pool, graphics_queue, {graphics_queue.family_index()}, mesh);
                m_vertex_count = mesh.points.size();

                m_buffers[0] = *m_vertex_buffer;
                m_offsets[0] = 0;
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
        }
};

MeshObject::MeshObject(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const mesh::MeshObject<3>& mesh_object,
        const std::function<vulkan::Descriptors(const std::vector<MaterialInfo>& materials)>& create_descriptor_sets)
        : m_model_matrix(mesh_object.matrix())
{
        if (!mesh_object.mesh().facets.empty())
        {
                m_triangles = std::make_unique<MeshObject::Triangles>(
                        device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                        mesh_object.mesh(), create_descriptor_sets);
        }

        if (!mesh_object.mesh().lines.empty())
        {
                m_lines = std::make_unique<MeshObject::Lines>(
                        device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                        mesh_object.mesh());
        }

        if (!mesh_object.mesh().points.empty())
        {
                m_points = std::make_unique<MeshObject::Points>(
                        device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                        mesh_object.mesh());
        }
}

MeshObject::~MeshObject() = default;

const mat4& MeshObject::model_matrix() const
{
        return m_model_matrix;
}

void MeshObject::commands_triangles(
        VkCommandBuffer buffer,
        VkDescriptorSetLayout material_descriptor_set_layout,
        const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const
{
        if (m_triangles)
        {
                m_triangles->draw_commands(buffer, material_descriptor_set_layout, bind_material_descriptor_set);
        }
}

void MeshObject::commands_plain_triangles(VkCommandBuffer buffer) const
{
        if (m_triangles)
        {
                m_triangles->draw_commands_plain(buffer);
        }
}

void MeshObject::commands_triangle_vertices(VkCommandBuffer buffer) const
{
        if (m_triangles)
        {
                m_triangles->draw_commands_vertices(buffer);
        }
}

void MeshObject::commands_lines(VkCommandBuffer buffer) const
{
        if (m_lines)
        {
                m_lines->draw_commands(buffer);
        }
}

void MeshObject::commands_points(VkCommandBuffer buffer) const
{
        if (m_points)
        {
                m_points->draw_commands(buffer);
        }
}
}
