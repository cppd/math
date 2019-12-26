/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "draw_object.h"

#include "shader_points.h"
#include "shader_triangles.h"

#include "com/container.h"
#include "com/error.h"
#include "com/hash.h"
#include "com/log.h"
#include "com/time.h"
#include "graphics/vulkan/buffers.h"
#include "obj/alg/alg.h"

#include <array>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// clang-format off
constexpr std::initializer_list<VkFormat> COLOR_IMAGE_FORMATS =
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

namespace gpu_vulkan
{
// Число используется в шейдере для определения наличия текстурных координат
constexpr vec2f NO_TEXTURE_COORDINATES = vec2f(-1e10);

constexpr VkIndexType VULKAN_INDEX_TYPE = VK_INDEX_TYPE_UINT32;
using IndexType = std::conditional_t<VULKAN_INDEX_TYPE == VK_INDEX_TYPE_UINT32, uint32_t,
                                     std::conditional_t<VULKAN_INDEX_TYPE == VK_INDEX_TYPE_UINT16, uint16_t, void>>;

namespace
{
std::string time_string(double time)
{
        return to_string_fixed(1000.0 * time, 5) + " ms";
}

struct Hash
{
        size_t operator()(const RendererTrianglesVertex& v) const
        {
                static_assert(sizeof(uint64_t) * 4 == sizeof(RendererTrianglesVertex));
                const uint64_t* s = reinterpret_cast<const uint64_t*>(&v);
                return pack_hash(s[0], s[1], s[2], s[3]);
        }
};
struct Equal
{
        bool operator()(const RendererTrianglesVertex& v1, const RendererTrianglesVertex& v2) const
        {
                return v1.position == v2.position && v1.normal == v2.normal && v1.texture_coordinates == v2.texture_coordinates;
        }
};

void load_vertices(const vulkan::Device& device, const vulkan::CommandPool& transfer_command_pool,
                   const vulkan::Queue& transfer_queue, const std::unordered_set<uint32_t>& family_indices, const Obj<3>& obj,
                   const std::vector<int>& sorted_face_indices, std::unique_ptr<vulkan::BufferWithMemory>& vertex_buffer,
                   std::unique_ptr<vulkan::BufferWithMemory>& index_buffer)
{
        if (obj.facets().size() == 0)
        {
                error("No OBJ facets found");
        }

        ASSERT(sorted_face_indices.size() == obj.facets().size());

        //

        double create_time = time_in_seconds();

        const std::vector<Obj<3>::Facet>& obj_faces = obj.facets();
        const std::vector<vec3f>& obj_vertices = obj.vertices();
        const std::vector<vec3f>& obj_normals = obj.normals();
        const std::vector<vec2f>& obj_texcoords = obj.texcoords();

        std::vector<RendererTrianglesVertex> vertices;
        std::vector<IndexType> indices;
        std::unordered_map<RendererTrianglesVertex, unsigned, Hash, Equal> map;
        vertices.reserve(3 * obj_faces.size());
        indices.reserve(3 * obj_faces.size());
        map.reserve(3 * obj_faces.size());

        std::array<vec3f, 3> v;
        std::array<vec3f, 3> n;
        std::array<vec2f, 3> t;

        for (int face_index : sorted_face_indices)
        {
                const Obj<3>::Facet& f = obj_faces[face_index];

                for (int i = 0; i < 3; ++i)
                {
                        v[i] = obj_vertices[f.vertices[i]];
                }

                if (f.has_normal)
                {
                        for (int i = 0; i < 3; ++i)
                        {
                                n[i] = obj_normals[f.normals[i]];
                        }
                }
                else
                {
                        vec3f geometric_normal = normalize(cross(v[1] - v[0], v[2] - v[0]));
                        if (!is_finite(geometric_normal))
                        {
                                error("Face unit orthogonal vector is not finite for the face with vertices (" + to_string(v[0]) +
                                      ", " + to_string(v[1]) + ", " + to_string(v[2]) + ")");
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
                                t[i] = obj_texcoords[f.texcoords[i]];
                        }
                }
                else
                {
                        for (int i = 0; i < 3; ++i)
                        {
                                t[i] = NO_TEXTURE_COORDINATES;
                        }
                }

                for (int i = 0; i < 3; ++i)
                {
                        RendererTrianglesVertex vertex(v[i], n[i], t[i]);
                        const auto [iter, inserted] = map.emplace(vertex, map.size());
                        if (inserted)
                        {
                                vertices.push_back(vertex);
                        }
                        indices.push_back(iter->second);
                }
        }

        ASSERT((indices.size() >= 3) && (indices.size() % 3 == 0));

        create_time = time_in_seconds() - create_time;

        //

        double load_time = time_in_seconds();

        vertex_buffer =
                std::make_unique<vulkan::BufferWithMemory>(device, transfer_command_pool, transfer_queue, family_indices,
                                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size(vertices), vertices);
        index_buffer = std::make_unique<vulkan::BufferWithMemory>(device, transfer_command_pool, transfer_queue, family_indices,
                                                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data_size(indices), indices);

        load_time = time_in_seconds() - load_time;

        //

        std::ostringstream oss;
        oss << "creating = " << time_string(create_time) << ", loading = " << time_string(load_time);
        oss << ", vertices = " << vertices.size() << " (" << data_size(vertices) << " bytes)";
        oss << ", faces = " << indices.size() / 3 << " (" << data_size(indices) << " bytes)";
        LOG(oss.str());
}

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(const vulkan::Device& device,
                                                              const vulkan::CommandPool& transfer_command_pool,
                                                              const vulkan::Queue& transfer_queue,
                                                              const std::unordered_set<uint32_t>& family_indices,
                                                              const Obj<3>& obj)
{
        if (obj.points().size() == 0)
        {
                error("No OBJ points found");
        }

        const std::vector<Obj<3>::Point>& obj_points = obj.points();
        const std::vector<vec3f>& obj_vertices = obj.vertices();

        std::vector<RendererPointsVertex> vertices;
        vertices.reserve(obj_points.size());

        for (const Obj<3>::Point& p : obj_points)
        {
                vertices.push_back(obj_vertices[p.vertex]);
        }

        return std::make_unique<vulkan::BufferWithMemory>(device, transfer_command_pool, transfer_queue, family_indices,
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size(vertices), vertices);
}

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(const vulkan::Device& device,
                                                             const vulkan::CommandPool& transfer_command_pool,
                                                             const vulkan::Queue& transfer_queue,
                                                             const std::unordered_set<uint32_t>& family_indices,
                                                             const Obj<3>& obj)
{
        if (obj.lines().size() == 0)
        {
                error("No OBJ lines found");
        }

        const std::vector<Obj<3>::Line>& obj_lines = obj.lines();
        const std::vector<vec3f>& obj_vertices = obj.vertices();

        std::vector<RendererPointsVertex> vertices;
        vertices.reserve(2 * obj_lines.size());

        for (const Obj<3>::Line& line : obj_lines)
        {
                for (int index : line.vertices)
                {
                        vertices.push_back(obj_vertices[index]);
                }
        }

        return std::make_unique<vulkan::BufferWithMemory>(device, transfer_command_pool, transfer_queue, family_indices,
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size(vertices), vertices);
}

std::vector<vulkan::ImageWithMemory> load_textures(const vulkan::Device& device, const vulkan::CommandPool& graphics_command_pool,
                                                   const vulkan::Queue& graphics_queue,
                                                   const vulkan::CommandPool& transfer_command_pool,
                                                   const vulkan::Queue& transfer_queue,
                                                   const std::unordered_set<uint32_t>& family_indices, const Obj<3>& obj)
{
        constexpr bool storage = false;

        std::vector<vulkan::ImageWithMemory> textures;

        for (const typename Obj<3>::Image& image : obj.images())
        {
                textures.emplace_back(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                                      family_indices, COLOR_IMAGE_FORMATS, image.size[0], image.size[1],
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.srgba_pixels, storage);
                ASSERT(textures.back().usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT(!(textures.back().usage() & VK_IMAGE_USAGE_STORAGE_BIT));
        }

        // На одну текстуру больше для её указания, но не использования в тех материалах, где нет текстуры
        std::vector<std::uint_least8_t> pixels = {/*0*/ 0, 0, 0, 0, /*1*/ 0, 0, 0, 0,
                                                  /*2*/ 0, 0, 0, 0, /*3*/ 0, 0, 0, 0};
        textures.emplace_back(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                              family_indices, COLOR_IMAGE_FORMATS, 2, 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pixels,
                              storage);
        ASSERT(textures.back().usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
        ASSERT(!(textures.back().usage() & VK_IMAGE_USAGE_STORAGE_BIT));

        return textures;
}

std::unique_ptr<RendererTrianglesMaterialMemory> load_materials(const vulkan::Device& device,
                                                                const std::unordered_set<uint32_t>& family_indices,
                                                                VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                                                                const Obj<3>& obj,
                                                                const std::vector<vulkan::ImageWithMemory>& textures)
{
        // Текстур имеется больше на одну для её использования в тех материалах, где нет текстуры

        ASSERT(textures.size() == obj.images().size() + 1);

        const vulkan::ImageWithMemory* const no_texture = &textures.back();

        std::vector<RendererTrianglesMaterialMemory::MaterialAndTexture> materials;
        materials.reserve(obj.materials().size() + 1);

        for (const typename Obj<3>::Material& material : obj.materials())
        {
                ASSERT(material.map_Ka < static_cast<int>(textures.size()) - 1);
                ASSERT(material.map_Kd < static_cast<int>(textures.size()) - 1);
                ASSERT(material.map_Ks < static_cast<int>(textures.size()) - 1);

                RendererTrianglesMaterialMemory::MaterialAndTexture m;

                m.material.Ka = material.Ka.to_rgb_vector<float>();
                m.material.Kd = material.Kd.to_rgb_vector<float>();
                m.material.Ks = material.Ks.to_rgb_vector<float>();

                m.material.Ns = material.Ns;

                m.material.use_texture_Ka = (material.map_Ka >= 0) ? 1 : 0;
                m.texture_Ka = (material.map_Ka >= 0) ? &textures[material.map_Ka] : no_texture;

                m.material.use_texture_Kd = (material.map_Kd >= 0) ? 1 : 0;
                m.texture_Kd = (material.map_Kd >= 0) ? &textures[material.map_Kd] : no_texture;

                m.material.use_texture_Ks = (material.map_Ks >= 0) ? 1 : 0;
                m.texture_Ks = (material.map_Ks >= 0) ? &textures[material.map_Ks] : no_texture;

                m.material.use_material = 1;

                materials.push_back(m);
        }

        // На один материал больше для его указания, но не использования в вершинах, не имеющих материала
        RendererTrianglesMaterialMemory::MaterialAndTexture m;
        m.material.Ka = vec3f(0);
        m.material.Kd = vec3f(0);
        m.material.Ks = vec3f(0);
        m.material.Ns = 0;
        m.material.use_texture_Ka = 0;
        m.texture_Ka = no_texture;
        m.material.use_texture_Kd = 0;
        m.texture_Kd = no_texture;
        m.material.use_texture_Ks = 0;
        m.texture_Ks = no_texture;
        m.material.use_material = 0;
        materials.push_back(m);

        return std::make_unique<RendererTrianglesMaterialMemory>(device, family_indices, sampler, descriptor_set_layout,
                                                                 materials);
}
}

class DrawObject::Triangles final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        std::unique_ptr<vulkan::BufferWithMemory> m_index_buffer;
        std::vector<vulkan::ImageWithMemory> m_textures;
        std::unique_ptr<RendererTrianglesMaterialMemory> m_shader_memory;
        unsigned m_vertex_count;

        //

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

        struct Material
        {
                VkDescriptorSet descriptor_set;
                unsigned vertex_offset;
                unsigned vertex_count;

                Material(VkDescriptorSet descriptor_set_, unsigned vertex_offset_, unsigned vertex_count_)
                        : descriptor_set(descriptor_set_), vertex_offset(vertex_offset_), vertex_count(vertex_count_)
                {
                }
        };
        std::vector<Material> m_materials;

public:
        Triangles(const vulkan::Device& device, const vulkan::CommandPool& graphics_command_pool,
                  const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
                  const vulkan::Queue& transfer_queue, VkSampler sampler,
                  VkDescriptorSetLayout triangles_material_descriptor_set_layout, const Obj<3>& obj)
        {
                ASSERT(obj.facets().size() > 0);

                std::vector<int> sorted_face_indices;
                std::vector<int> material_face_offset;
                std::vector<int> material_face_count;

                sort_facets_by_material(obj, sorted_face_indices, material_face_offset, material_face_count);

                load_vertices(device, transfer_command_pool, transfer_queue,
                              {graphics_queue.family_index(), transfer_queue.family_index()}, obj, sorted_face_indices,
                              m_vertex_buffer, m_index_buffer);

                m_textures = load_textures(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                                           {graphics_queue.family_index(), transfer_queue.family_index()}, obj);

                m_shader_memory = load_materials(device, {graphics_queue.family_index()}, sampler,
                                                 triangles_material_descriptor_set_layout, obj, m_textures);

                m_vertex_count = 3 * obj.facets().size();

                ASSERT(material_face_offset.size() == material_face_count.size());
                ASSERT(material_face_offset.size() == m_shader_memory->descriptor_set_count());

                for (unsigned i = 0; i < m_shader_memory->descriptor_set_count(); ++i)
                {
                        if (material_face_count[i] > 0)
                        {
                                m_materials.emplace_back(m_shader_memory->descriptor_set(i), 3 * material_face_offset[i],
                                                         3 * material_face_count[i]);
                        }
                }

                m_buffers = {*m_vertex_buffer};
                m_offsets = {0};
        }

        void draw_commands(VkCommandBuffer command_buffer, const DrawInfo& info) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.triangles_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.triangles_pipeline_layout,
                                        info.triangles_shared_set_number, 1 /*set count*/, &info.triangles_shared_set, 0,
                                        nullptr);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_index_buffer, 0, VULKAN_INDEX_TYPE);

                for (const Material& material : m_materials)
                {
                        ASSERT(material.vertex_count > 0);

                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.triangles_pipeline_layout,
                                                RendererTrianglesMaterialMemory::set_number(), 1 /*set count*/,
                                                &material.descriptor_set, 0, nullptr);

                        vkCmdDrawIndexed(command_buffer, material.vertex_count, 1, material.vertex_offset, 0, 0);
                }
        }

        void shadow_commands(VkCommandBuffer command_buffer, const ShadowInfo& info) const
        {
                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.triangles_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.triangles_pipeline_layout,
                                        info.triangles_set_number, 1 /*set count*/, &info.triangles_set, 0, nullptr);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());
                vkCmdBindIndexBuffer(command_buffer, *m_index_buffer, 0, VULKAN_INDEX_TYPE);

                vkCmdDrawIndexed(command_buffer, m_vertex_count, 1, 0, 0, 0);
        }
};

class DrawObject::Lines final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        unsigned m_vertex_count;

        //

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

public:
        Lines(const vulkan::Device& device, const vulkan::CommandPool& /*graphics_command_pool*/,
              const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
              const vulkan::Queue& transfer_queue, const Obj<3>& obj)
        {
                ASSERT(obj.lines().size() > 0);

                m_vertex_buffer = load_line_vertices(device, transfer_command_pool, transfer_queue,
                                                     {graphics_queue.family_index(), transfer_queue.family_index()}, obj);
                m_vertex_count = 2 * obj.lines().size();

                m_buffers = {*m_vertex_buffer};
                m_offsets = {0};
        }

        void draw_commands(VkCommandBuffer command_buffer, const DrawInfo& info) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.lines_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.lines_pipeline_layout,
                                        info.lines_set_number, 1 /*set count*/, &info.lines_set, 0, nullptr);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
        }
};

class DrawObject::Points final
{
        std::unique_ptr<vulkan::BufferWithMemory> m_vertex_buffer;
        unsigned m_vertex_count;

        //

        std::array<VkBuffer, 1> m_buffers;
        std::array<VkDeviceSize, 1> m_offsets;

public:
        Points(const vulkan::Device& device, const vulkan::CommandPool& /*graphics_command_pool*/,
               const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
               const vulkan::Queue& transfer_queue, const Obj<3>& obj)
        {
                ASSERT(obj.points().size() > 0);

                m_vertex_buffer = load_point_vertices(device, transfer_command_pool, transfer_queue,
                                                      {graphics_queue.family_index(), transfer_queue.family_index()}, obj);
                m_vertex_count = obj.points().size();

                m_buffers = {*m_vertex_buffer};
                m_offsets = {0};
        }

        void draw_commands(VkCommandBuffer command_buffer, const DrawInfo& info) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.points_pipeline);

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.points_pipeline_layout,
                                        info.points_set_number, 1 /*set count*/, &info.points_set, 0, nullptr);

                vkCmdBindVertexBuffers(command_buffer, 0, m_buffers.size(), m_buffers.data(), m_offsets.data());

                vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
        }
};

DrawObject::DrawObject(const vulkan::Device& device, const vulkan::CommandPool& graphics_command_pool,
                       const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
                       const vulkan::Queue& transfer_queue, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                       const Obj<3>& obj, double size, const vec3& position)
        : m_model_matrix(model_vertex_matrix(obj, size, position))
{
        if (obj.facets().size() > 0)
        {
                m_triangles = std::make_unique<DrawObject::Triangles>(device, graphics_command_pool, graphics_queue,
                                                                      transfer_command_pool, transfer_queue, sampler,
                                                                      descriptor_set_layout, obj);
        }

        if (obj.lines().size() > 0)
        {
                m_lines = std::make_unique<DrawObject::Lines>(device, graphics_command_pool, graphics_queue,
                                                              transfer_command_pool, transfer_queue, obj);
        }

        if (obj.points().size() > 0)
        {
                m_points = std::make_unique<DrawObject::Points>(device, graphics_command_pool, graphics_queue,
                                                                transfer_command_pool, transfer_queue, obj);
        }
}

DrawObject::~DrawObject() = default;

bool DrawObject::has_shadow() const
{
        return m_triangles.get() != nullptr;
}

const mat4& DrawObject::model_matrix() const
{
        return m_model_matrix;
}

void DrawObject::draw_commands(VkCommandBuffer command_buffer, const DrawInfo& info) const
{
        if (m_triangles)
        {
                m_triangles->draw_commands(command_buffer, info);
        }
        if (m_lines)
        {
                m_lines->draw_commands(command_buffer, info);
        }
        if (m_points)
        {
                m_points->draw_commands(command_buffer, info);
        }
}

void DrawObject::shadow_commands(VkCommandBuffer command_buffer, const ShadowInfo& info) const
{
        if (m_triangles)
        {
                m_triangles->shadow_commands(command_buffer, info);
        }
}
}
