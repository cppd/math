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

#include "shaders/buffers.h"
#include "shaders/descriptors.h"
#include "shaders/triangles.h"
#include "shaders/vertex_points.h"
#include "shaders/vertex_triangles.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/model/mesh_utility.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>

#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

constexpr float LIMIT_COSINE = 0.7; // 0.7 немного больше 45 градусов

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
                vertex_buffer->reset();
                index_buffer->reset();
                *vertex_count = 0;
                *index_count = 0;
                return;
        }

        ASSERT(sorted_face_indices.size() == mesh.facets.size());

        //

        TimePoint create_start_time = time();

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

                        const vec3f geometric_normal = cross(p[1] - p[0], p[2] - p[0]).normalized();
                        if (!is_finite(geometric_normal))
                        {
                                error("Face unit orthogonal vector is not finite for the face with vertices ("
                                      + to_string(p[0]) + ", " + to_string(p[1]) + ", " + to_string(p[2]) + ")");
                        }
                        if (f.has_normal)
                        {
                                std::array<float, 3> dots;
                                for (unsigned i = 0; i < 3; ++i)
                                {
                                        dots[i] = dot(mesh.normals[f.normals[i]], geometric_normal);
                                }
                                if (std::all_of(dots.cbegin(), dots.cend(), [](float d) {
                                            static_assert(LIMIT_COSINE > 0);
                                            return is_finite(d) && std::abs(d) >= LIMIT_COSINE;
                                    }))
                                {
                                        for (int i = 0; i < 3; ++i)
                                        {
                                                n[i] = mesh.normals[f.normals[i]];
                                        }
                                }
                                else
                                {
                                        for (int i = 0; i < 3; ++i)
                                        {
                                                n[i] = geometric_normal;
                                        }
                                }
                        }
                        else
                        {
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

        double create_duration = duration_from(create_start_time);

        //

        TimePoint map_start_time = time();

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

        double map_duration = duration_from(map_start_time);

        //

        TimePoint load_start_time = time();

        *vertex_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));
        (*vertex_buffer)->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        *index_buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(indices));
        (*index_buffer)->write(command_pool, queue, data_size(indices), data_pointer(indices));

        *vertex_count = vertices.size();
        *index_count = indices.size();

        double load_duration = duration_from(load_start_time);

        //

        std::ostringstream oss;
        oss << "create = " << time_string(create_duration);
        oss << ", map = " << time_string(map_duration);
        oss << ", load = " << time_string(load_duration);
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
                return std::unique_ptr<vulkan::BufferWithMemory>();
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(mesh.points.size());

        for (const mesh::Mesh<3>::Point& p : mesh.points)
        {
                vertices.emplace_back(mesh.vertices[p.vertex]);
        }

        std::unique_ptr<vulkan::BufferWithMemory> buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));

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
                return std::unique_ptr<vulkan::BufferWithMemory>();
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
                vulkan::BufferMemoryType::DeviceLocal, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));

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
        std::vector<vulkan::ImageWithMemory> textures;

        for (const image::Image<2>& image : mesh.images)
        {
                textures.emplace_back(
                        device, command_pool, queue, family_indices, COLOR_IMAGE_FORMATS, VK_SAMPLE_COUNT_1_BIT,
                        VK_IMAGE_TYPE_2D, vulkan::make_extent(image.size[0], image.size[1]), VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
                textures.back().write_pixels(
                        command_pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        image.color_format, image.pixels);
        }

        // На одну текстуру больше для её указания, но не использования в тех материалах, где нет текстуры
        textures.emplace_back(
                device, command_pool, queue, family_indices, COLOR_IMAGE_FORMATS, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TYPE_2D, vulkan::make_extent(1, 1), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT);

        return textures;
}

std::vector<MaterialBuffer> load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh)
{
        std::vector<MaterialBuffer> buffers;
        buffers.reserve(mesh.materials.size() + 1);

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

        return buffers;
}

std::vector<TrianglesMaterialMemory::MaterialInfo> materials_info(
        const mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        const std::vector<MaterialBuffer>& material_buffers)
{
        // Текстур имеется больше на одну для её использования в тех материалах, где нет текстуры
        ASSERT(textures.size() == mesh.images.size() + 1);

        // Буферов имеется больше на один для его указания, но не использования в вершинах, не имеющих материала
        ASSERT(material_buffers.size() == mesh.materials.size() + 1);

        const VkImageView no_texture = textures.back().image_view();

        std::vector<TrianglesMaterialMemory::MaterialInfo> materials;
        materials.reserve(mesh.materials.size() + 1);

        for (unsigned i = 0; i < mesh.materials.size(); ++i)
        {
                const typename mesh::Mesh<3>::Material& mesh_material = mesh.materials[i];

                ASSERT(mesh_material.map_Ka < static_cast<int>(textures.size()) - 1);
                ASSERT(mesh_material.map_Kd < static_cast<int>(textures.size()) - 1);
                ASSERT(mesh_material.map_Ks < static_cast<int>(textures.size()) - 1);

                TrianglesMaterialMemory::MaterialInfo& m = materials.emplace_back();
                m.buffer = material_buffers[i].buffer();
                m.buffer_size = material_buffers[i].buffer().size();
                m.texture_Ka = (mesh_material.map_Ka >= 0) ? textures[mesh_material.map_Ka].image_view() : no_texture;
                m.texture_Kd = (mesh_material.map_Kd >= 0) ? textures[mesh_material.map_Kd].image_view() : no_texture;
                m.texture_Ks = (mesh_material.map_Ks >= 0) ? textures[mesh_material.map_Ks].image_view() : no_texture;
        }

        TrianglesMaterialMemory::MaterialInfo& m = materials.emplace_back();
        m.buffer = material_buffers.back().buffer();
        m.buffer_size = material_buffers.back().buffer().size();
        m.texture_Ka = no_texture;
        m.texture_Kd = no_texture;
        m.texture_Ks = no_texture;

        return materials;
}

class Impl final : public MeshObject
{
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;

        MeshBuffer m_mesh_buffer;
        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> m_mesh_descriptor_sets;
        std::vector<vulkan::DescriptorSetLayoutAndBindings> m_mesh_layouts;

        struct MaterialVertices
        {
                int offset;
                int count;
        };
        std::vector<MaterialVertices> m_material_vertices;

        std::unique_ptr<vulkan::BufferWithMemory> m_faces_vertex_buffer;
        std::unique_ptr<vulkan::BufferWithMemory> m_faces_index_buffer;
        unsigned m_faces_vertex_count = 0;
        unsigned m_faces_index_count = 0;

        std::vector<vulkan::ImageWithMemory> m_textures;
        std::vector<MaterialBuffer> m_material_buffers;
        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> m_material_descriptor_sets;
        std::vector<vulkan::DescriptorSetLayoutAndBindings> m_material_layouts;
        VkSampler m_texture_sampler;

        std::unique_ptr<vulkan::BufferWithMemory> m_lines_vertex_buffer;
        unsigned m_lines_vertex_count = 0;

        std::unique_ptr<vulkan::BufferWithMemory> m_points_vertex_buffer;
        unsigned m_points_vertex_count = 0;

        bool m_transparent = false;

        std::optional<int> m_version;

        const mesh::Update::Flags UPDATE_LIGHTING = []() {
                mesh::Update::Flags flags;
                flags.set(mesh::Update::Ambient);
                flags.set(mesh::Update::Diffuse);
                flags.set(mesh::Update::Specular);
                flags.set(mesh::Update::SpecularPower);
                return flags;
        }();

        void buffer_set_lighting(float ambient, float diffuse, float specular, float specular_power) const
        {
                ambient = std::max(0.0f, ambient);
                diffuse = std::max(0.0f, diffuse);
                specular = std::max(0.0f, specular);
                specular_power = std::max(1.0f, specular_power);

                m_mesh_buffer.set_lighting(ambient, diffuse, specular, specular_power);
        }

        void buffer_set_color(const Color& color)
        {
                m_mesh_buffer.set_color(color);
        }

        void buffer_set_alpha(float alpha)
        {
                alpha = std::clamp(alpha, 0.0f, 1.0f);
                m_mesh_buffer.set_alpha(alpha);
        }

        void buffer_set_coordinates(const mat4& model_matrix)
        {
                m_mesh_buffer.set_coordinates(model_matrix, model_matrix.top_left<3, 3>().inverse().transpose());
        }

        void create_mesh_descriptor_sets()
        {
                m_mesh_descriptor_sets.clear();
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : m_mesh_layouts)
                {
                        vulkan::Descriptors sets = MeshMemory::create(
                                m_device, layout.descriptor_set_layout, layout.descriptor_set_layout_bindings,
                                {&m_mesh_buffer.buffer()});
                        ASSERT(sets.descriptor_set_count() == 1);
                        m_mesh_descriptor_sets.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        VkDescriptorSet find_mesh_descriptor_set(VkDescriptorSetLayout mesh_descriptor_set_layout) const
        {
                auto iter = m_mesh_descriptor_sets.find(mesh_descriptor_set_layout);
                if (iter == m_mesh_descriptor_sets.cend())
                {
                        error("Failed to find mesh descriptor sets for mesh descriptor set layout");
                }
                ASSERT(iter->second.descriptor_set_count() == 1);
                return iter->second.descriptor_set(0);
        }

        void create_material_descriptor_sets(const std::vector<TrianglesMaterialMemory::MaterialInfo>& material_info)
        {
                m_material_descriptor_sets.clear();
                if (material_info.empty())
                {
                        return;
                }
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : m_material_layouts)
                {
                        vulkan::Descriptors sets = TrianglesMaterialMemory::create(
                                m_device, m_texture_sampler, layout.descriptor_set_layout,
                                layout.descriptor_set_layout_bindings, material_info);
                        ASSERT(sets.descriptor_set_count() == material_info.size());
                        m_material_descriptor_sets.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        const vulkan::Descriptors& find_material_descriptor_sets(
                VkDescriptorSetLayout material_descriptor_set_layout) const
        {
                auto iter = m_material_descriptor_sets.find(material_descriptor_set_layout);
                if (iter == m_material_descriptor_sets.cend())
                {
                        error("Failed to find material descriptor sets for material descriptor set layout");
                }

                ASSERT(iter->second.descriptor_set_count() == m_material_vertices.size());

                return iter->second;
        }

        //

        void load_mesh_textures_and_materials(const mesh::Mesh<3>& mesh)
        {
                if (mesh.facets.empty())
                {
                        m_textures.clear();
                        m_material_buffers.clear();
                        create_material_descriptor_sets({});
                        return;
                }

                m_textures = load_textures(
                        m_device, m_graphics_command_pool, m_graphics_queue, {m_graphics_queue.family_index()}, mesh);

                m_material_buffers = load_materials(
                        m_device, m_graphics_command_pool, m_graphics_queue, {m_graphics_queue.family_index()}, mesh);

                create_material_descriptor_sets(materials_info(mesh, m_textures, m_material_buffers));
        }

        void load_mesh_vertices(const mesh::Mesh<3>& mesh)
        {
                {
                        std::vector<int> sorted_face_indices;
                        std::vector<int> material_face_offset;
                        std::vector<int> material_face_count;

                        sort_facets_by_material(
                                mesh, &sorted_face_indices, &material_face_offset, &material_face_count);

                        ASSERT(material_face_offset.size() == material_face_count.size());
                        ASSERT(std::all_of(
                                m_material_descriptor_sets.cbegin(), m_material_descriptor_sets.cend(),
                                [&](const auto& v) {
                                        return material_face_offset.size() == v.second.descriptor_set_count();
                                }));

                        m_material_vertices.resize(material_face_count.size());
                        for (unsigned i = 0; i < material_face_count.size(); ++i)
                        {
                                m_material_vertices[i].offset = 3 * material_face_offset[i];
                                m_material_vertices[i].count = 3 * material_face_count[i];
                        }

                        load_vertices(
                                m_device, m_graphics_command_pool, m_graphics_queue, {m_graphics_queue.family_index()},
                                mesh, sorted_face_indices, &m_faces_vertex_buffer, &m_faces_index_buffer,
                                &m_faces_vertex_count, &m_faces_index_count);

                        ASSERT(m_faces_index_count == 3 * mesh.facets.size());
                }

                {
                        m_lines_vertex_buffer = load_line_vertices(
                                m_device, m_graphics_command_pool, m_graphics_queue, {m_graphics_queue.family_index()},
                                mesh);
                        m_lines_vertex_count = 2 * mesh.lines.size();
                }

                {
                        m_points_vertex_buffer = load_point_vertices(
                                m_device, m_graphics_command_pool, m_graphics_queue, {m_graphics_queue.family_index()},
                                mesh);
                        m_points_vertex_count = mesh.points.size();
                }
        }

        //

        bool transparent() const override
        {
                return m_transparent;
        }

        void commands_triangles(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set,
                VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const override
        {
                if (m_faces_vertex_count == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const vulkan::Descriptors& descriptor_sets =
                        find_material_descriptor_sets(material_descriptor_set_layout);

                const std::array<VkBuffer, 1> buffers{*m_faces_vertex_buffer};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_faces_index_buffer, 0, VULKAN_INDEX_TYPE);

                for (unsigned i = 0; i < m_material_vertices.size(); ++i)
                {
                        if (m_material_vertices[i].count <= 0)
                        {
                                continue;
                        }

                        bind_material_descriptor_set(descriptor_sets.descriptor_set(i));

                        vkCmdDrawIndexed(
                                command_buffer, m_material_vertices[i].count, 1, m_material_vertices[i].offset, 0, 0);
                }
        }

        void commands_plain_triangles(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (m_faces_vertex_count == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{*m_faces_vertex_buffer};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_faces_index_buffer, 0, VULKAN_INDEX_TYPE);

                vkCmdDrawIndexed(command_buffer, m_faces_index_count, 1, 0, 0, 0);
        }

        void commands_triangle_vertices(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (m_faces_vertex_count == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{*m_faces_vertex_buffer};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, m_faces_vertex_count, 1, 0, 0);
        }

        void commands_lines(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (m_lines_vertex_count == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{*m_lines_vertex_buffer};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, m_lines_vertex_count, 1, 0, 0);
        }

        void commands_points(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (m_points_vertex_count == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{*m_points_vertex_buffer};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, m_points_vertex_count, 1, 0, 0);
        }

        UpdateChanges update(const mesh::Reading<3>& mesh_object) override
        {
                UpdateChanges update_changes;

                mesh::Update::Flags updates;
                mesh_object.updates(&m_version, &updates);
                if (updates.none())
                {
                        return update_changes;
                }

                ASSERT(!mesh_object.mesh().facets.empty() || !mesh_object.mesh().lines.empty()
                       || !mesh_object.mesh().points.empty());

                static_assert(mesh::Update::Flags().size() == 8);

                bool update_mesh = updates[mesh::Update::Mesh];
                bool update_matrix = updates[mesh::Update::Matrix];
                bool update_color = updates[mesh::Update::Color];
                bool update_alpha = updates[mesh::Update::Alpha];
                bool update_lighting = (updates & UPDATE_LIGHTING).any();

                if (update_matrix)
                {
                        buffer_set_coordinates(mesh_object.matrix());
                }

                if (update_alpha)
                {
                        buffer_set_alpha(mesh_object.alpha());

                        bool transparent = mesh_object.alpha() < 1;
                        if (m_transparent != transparent)
                        {
                                m_transparent = transparent;
                                update_changes.transparency = true;
                        }
                }

                if (update_color)
                {
                        buffer_set_color(mesh_object.color());
                }

                if (update_lighting)
                {
                        buffer_set_lighting(
                                mesh_object.ambient(), mesh_object.diffuse(), mesh_object.specular(),
                                mesh_object.specular_power());
                }

                if (update_mesh)
                {
                        const mesh::Mesh<3>& mesh = mesh_object.mesh();

                        load_mesh_textures_and_materials(mesh);
                        load_mesh_vertices(mesh);

                        update_changes.command_buffers = true;
                }

                return update_changes;
        }

public:
        Impl(const vulkan::Device& device,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& /*transfer_command_pool*/,
             const vulkan::Queue& /*transfer_queue*/,
             const std::vector<vulkan::DescriptorSetLayoutAndBindings>& mesh_layouts,
             const std::vector<vulkan::DescriptorSetLayoutAndBindings>& material_layouts,
             VkSampler texture_sampler)
                : m_device(device),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_mesh_buffer(device, {m_graphics_queue.family_index()}),
                  m_mesh_layouts(mesh_layouts),
                  m_material_layouts(material_layouts),
                  m_texture_sampler(texture_sampler)
        {
                create_mesh_descriptor_sets();
        }
};
}

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& mesh_layouts,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& material_layouts,
        VkSampler texture_sampler)
{
        return std::make_unique<Impl>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, mesh_layouts,
                material_layouts, texture_sampler);
}
}
