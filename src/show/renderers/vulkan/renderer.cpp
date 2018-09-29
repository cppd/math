/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "renderer.h"

#include "com/color/conversion.h"
#include "com/log.h"
#include "com/math.h"
#include "com/time.h"
#include "com/vec.h"
#include "graphics/vulkan/common.h"
#include "graphics/vulkan/device.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/query.h"
#include "graphics/vulkan/sampler.h"
#include "obj/obj_alg.h"
#include "show/renderers/com.h"
#include "show/renderers/vulkan/shader/shader.h"

#include <array>
#include <thread>

constexpr int API_VERSION_MAJOR = 1;
constexpr int API_VERSION_MINOR = 0;

constexpr std::array<const char*, 0> INSTANCE_EXTENSIONS = {};
constexpr std::array<const char*, 0> DEVICE_EXTENSIONS = {};
constexpr std::array<const char*, 1> VALIDATION_LAYERS = {"VK_LAYER_LUNARG_standard_validation"};

constexpr std::array<vulkan::PhysicalDeviceFeatures, 3> REQUIRED_FEATURES = {vulkan::PhysicalDeviceFeatures::GeometryShader,
                                                                             vulkan::PhysicalDeviceFeatures::SampleRateShading,
                                                                             vulkan::PhysicalDeviceFeatures::SamplerAnisotropy};
constexpr std::array<vulkan::PhysicalDeviceFeatures, 0> OPTIONAL_FEATURES = {};

// 2 - double buffering, 3 - triple buffering
constexpr int PREFERRED_IMAGE_COUNT = 2;
constexpr int MAX_FRAMES_IN_FLIGHT = 1;

constexpr int REQUIRED_MINIMUM_SAMPLE_COUNT = 4;

constexpr VkIndexType VULKAN_VERTEX_INDEX_TYPE = VK_INDEX_TYPE_UINT32;
using VERTEX_INDEX_TYPE =
        std::conditional_t<VULKAN_VERTEX_INDEX_TYPE == VK_INDEX_TYPE_UINT32, uint32_t,
                           std::conditional_t<VULKAN_VERTEX_INDEX_TYPE == VK_INDEX_TYPE_UINT16, uint16_t, void>>;

constexpr uint32_t SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX = 0;
constexpr uint32_t SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX = 1;

// Число используется в шейдере для определения наличия текстурных координат
constexpr vec2f NO_TEXTURE_COORDINATES = vec2f(-1e10);

// clang-format off
constexpr uint32_t vertex_shader[]
{
#include "renderer_triangles.vert.spr"
};
constexpr uint32_t geometry_shader[]
{
#include "renderer_triangles.geom.spr"
};
constexpr uint32_t fragment_shader[]
{
#include "renderer_triangles.frag.spr"
};
constexpr uint32_t shadow_vertex_shader[]
{
#include "renderer_shadow.vert.spr"
};
constexpr uint32_t shadow_fragment_shader[]
{
#include "renderer_shadow.frag.spr"
};
// clang-format on

namespace shaders = vulkan_renderer_shaders;

namespace
{
std::vector<std::string> instance_extensions()
{
        return std::vector<std::string>(std::cbegin(INSTANCE_EXTENSIONS), std::cend(INSTANCE_EXTENSIONS));
}
std::vector<std::string> device_extensions()
{
        return std::vector<std::string>(std::cbegin(DEVICE_EXTENSIONS), std::cend(DEVICE_EXTENSIONS));
}
std::vector<std::string> validation_layers()
{
        return std::vector<std::string>(std::cbegin(VALIDATION_LAYERS), std::cend(VALIDATION_LAYERS));
}
std::vector<vulkan::PhysicalDeviceFeatures> required_features()
{
        return std::vector<vulkan::PhysicalDeviceFeatures>(std::cbegin(REQUIRED_FEATURES), std::cend(REQUIRED_FEATURES));
}
std::vector<vulkan::PhysicalDeviceFeatures> optional_features()
{
        return std::vector<vulkan::PhysicalDeviceFeatures>(std::cbegin(OPTIONAL_FEATURES), std::cend(OPTIONAL_FEATURES));
}

struct VerticesOfMaterial
{
        int offset;
        int count;
};

template <size_t N>
void facets_sorted_by_material(const Obj<N>& obj, std::vector<int>& sorted_facet_indices,
                               std::vector<VerticesOfMaterial>& vertices_of_materials)
{
        std::vector<int> material_facet_offset;
        std::vector<int> material_facet_count;

        sort_facets_by_material(obj, sorted_facet_indices, material_facet_offset, material_facet_count);

        vertices_of_materials.resize(material_facet_offset.size());
        for (size_t i = 0; i < vertices_of_materials.size(); ++i)
        {
                vertices_of_materials[i].offset = N * material_facet_offset[i];
                vertices_of_materials[i].count = N * material_facet_count[i];
        }

        ASSERT(sorted_facet_indices.size() == obj.facets().size());
        ASSERT(vertices_of_materials.size() == obj.materials().size() + 1);
}

std::unique_ptr<vulkan::VertexBufferWithDeviceLocalMemory> load_vertices(const vulkan::VulkanInstance& instance,
                                                                         const Obj<3>& obj, std::vector<int> sorted_face_indices)
{
        if (obj.facets().size() == 0)
        {
                error("No OBJ facets found");
        }

        ASSERT(sorted_face_indices.size() == obj.facets().size());

        const std::vector<Obj<3>::Facet>& obj_faces = obj.facets();
        const std::vector<vec3f>& obj_vertices = obj.vertices();
        const std::vector<vec3f>& obj_normals = obj.normals();
        const std::vector<vec2f>& obj_texcoords = obj.texcoords();

        std::vector<shaders::Vertex> shader_vertices;
        shader_vertices.reserve(3 * obj_faces.size());

        vec3f v0, v1, v2;
        vec3f n0, n1, n2;
        vec2f t0, t1, t2;

        for (int face_index : sorted_face_indices)
        {
                const Obj<3>::Facet& f = obj_faces[face_index];

                v0 = obj_vertices[f.vertices[0]];
                v1 = obj_vertices[f.vertices[1]];
                v2 = obj_vertices[f.vertices[2]];

                vec3f geometric_normal = normalize(cross(v1 - v0, v2 - v0));
                if (!is_finite(geometric_normal))
                {
                        error("Face unit orthogonal vector is not finite for the face with vertices (" + to_string(v0) + ", " +
                              to_string(v1) + ", " + to_string(v2) + ")");
                }

                if (f.has_normal)
                {
                        n0 = obj_normals[f.normals[0]];
                        n1 = obj_normals[f.normals[1]];
                        n2 = obj_normals[f.normals[2]];

                        // Векторы вершин грани могут быть направлены в разные стороны от грани,
                        // поэтому надо им задать одинаковое направление
                        n0 = dot(n0, geometric_normal) >= 0 ? n0 : -n0;
                        n1 = dot(n1, geometric_normal) >= 0 ? n1 : -n1;
                        n2 = dot(n2, geometric_normal) >= 0 ? n2 : -n2;
                }
                else
                {
                        n0 = n1 = n2 = geometric_normal;
                }

                if (f.has_texcoord)
                {
                        t0 = obj_texcoords[f.texcoords[0]];
                        t1 = obj_texcoords[f.texcoords[1]];
                        t2 = obj_texcoords[f.texcoords[2]];
                }
                else
                {
                        t0 = t1 = t2 = NO_TEXTURE_COORDINATES;
                }

                shader_vertices.emplace_back(v0, n0, geometric_normal, t0);
                shader_vertices.emplace_back(v1, n1, geometric_normal, t1);
                shader_vertices.emplace_back(v2, n2, geometric_normal, t2);
        }

        ASSERT((shader_vertices.size() >= 3) && (shader_vertices.size() % 3 == 0));

        return std::make_unique<vulkan::VertexBufferWithDeviceLocalMemory>(instance.create_vertex_buffer(shader_vertices));
}

std::vector<uint16_t> integer_srgb_pixels_to_integer_rgb_pixels(const std::vector<unsigned char>& pixels)
{
        std::vector<uint16_t> buffer(pixels.size());
        for (size_t i = 0; i < buffer.size(); ++i)
        {
                buffer[i] = color_conversion::srgb_uint8_to_rgb_uint16(pixels[i]);
        }
        return buffer;
}

std::vector<vulkan::Texture> load_textures(const vulkan::VulkanInstance& instance, const Obj<3>& obj)
{
        std::vector<vulkan::Texture> textures;

        for (const typename Obj<3>::Image& image : obj.images())
        {
                textures.push_back(instance.create_texture(image.size[0], image.size[1],
                                                           integer_srgb_pixels_to_integer_rgb_pixels(image.srgba_pixels)));
        }

        // На одну текстуру больше для её указания, но не использования в тех материалах, где нет текстуры
        std::vector<std::uint16_t> pixels = {/*0*/ 0, 0, 0, 0, /*1*/ 0, 0, 0, 0,
                                             /*2*/ 0, 0, 0, 0, /*3*/ 0, 0, 0, 0};
        textures.push_back(instance.create_texture(2, 2, pixels));

        return textures;
}

std::unique_ptr<shaders::PerObjectMemory> load_materials(const vulkan::Device& device, VkSampler sampler,
                                                         VkDescriptorSetLayout descriptor_set_layout, const Obj<3>& obj,
                                                         const std::vector<vulkan::Texture>& textures)
{
        // Текстур имеется больше на одну для её использования в тех материалах, где нет текстуры

        ASSERT(textures.size() == obj.images().size() + 1);

        const vulkan::Texture* const no_texture = &textures.back();

        std::vector<shaders::PerObjectMemory::MaterialAndTexture> materials;
        materials.reserve(obj.materials().size() + 1);

        for (const typename Obj<3>::Material& material : obj.materials())
        {
                ASSERT(material.map_Ka < static_cast<int>(textures.size()) - 1);
                ASSERT(material.map_Kd < static_cast<int>(textures.size()) - 1);
                ASSERT(material.map_Ks < static_cast<int>(textures.size()) - 1);

                shaders::PerObjectMemory::MaterialAndTexture m;

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
        shaders::PerObjectMemory::MaterialAndTexture m;
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

        return std::make_unique<shaders::PerObjectMemory>(device, sampler, descriptor_set_layout, materials);
}

class DrawObject
{
        mat4 m_model_matrix;
        DrawType m_draw_type;

        std::unique_ptr<vulkan::VertexBufferWithDeviceLocalMemory> m_vertex_buffer;
        // std::unique_ptr<vulkan::IndexBufferWithDeviceLocalMemory> m_vertex_index_buffer;
        std::vector<VerticesOfMaterial> m_vertices_of_materials;
        std::vector<vulkan::Texture> m_textures;
        std::unique_ptr<shaders::PerObjectMemory> m_shader_memory;

public:
        DrawObject(const vulkan::VulkanInstance& instance, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                   const Obj<3>* obj, double size, const vec3& position)
                : m_model_matrix(model_vertex_matrix(obj, size, position)), m_draw_type(draw_type_of_obj(obj))
        {
                if (m_draw_type == DrawType::Triangles && obj->facets().size() > 0)
                {
                        std::vector<int> sorted_face_indices;
                        facets_sorted_by_material(*obj, sorted_face_indices, m_vertices_of_materials);
                        m_vertex_buffer = load_vertices(instance, *obj, sorted_face_indices);

                        ASSERT(m_vertex_buffer && m_vertices_of_materials.size() > 0);

                        m_textures = load_textures(instance, *obj);

                        m_shader_memory = load_materials(instance.device(), sampler, descriptor_set_layout, *obj, m_textures);
                }
        }

        const mat4& model_matrix() const noexcept
        {
                return m_model_matrix;
        }

        void draw_commands(VkPipelineLayout pipeline_layout, VkCommandBuffer command_buffer,
                           uint32_t description_set_layout_index) const
        {
                if (!m_vertex_buffer)
                {
                        return;
                }

                std::array<VkBuffer, 1> vertex_buffers = {*m_vertex_buffer};
                std::array<VkDeviceSize, vertex_buffers.size()> offsets = {0};
                vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

#if 1
                ASSERT(m_vertices_of_materials.size() == m_shader_memory->descriptor_set_count());

                for (size_t i = 0; i < m_vertices_of_materials.size(); ++i)
                {
                        if (m_vertices_of_materials[i].count <= 0)
                        {
                                continue;
                        }

                        std::array<VkDescriptorSet, 1> descriptor_sets = {m_shader_memory->descriptor_set(i)};
                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                                description_set_layout_index, descriptor_sets.size(), descriptor_sets.data(), 0,
                                                nullptr);

                        vkCmdDraw(command_buffer, m_vertices_of_materials[i].count, 1, m_vertices_of_materials[i].offset, 0);
                }
#else
                // Рисование с индексами вершин вместо самих вершин
                vkCmdBindIndexBuffer(command_buffer, *m_vertex_index_buffer, 0, VULKAN_VERTEX_INDEX_TYPE);
                vkCmdDrawIndexed(command_buffer, m_vertex_count, 1, 0, 0, 0);
#endif
        }

        void shadow_draw_commands(VkCommandBuffer command_buffer) const
        {
                if (!m_vertex_buffer)
                {
                        return;
                }

                std::array<VkBuffer, 1> vertex_buffers = {*m_vertex_buffer};
                std::array<VkDeviceSize, vertex_buffers.size()> offsets = {0};
                vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

                ASSERT(m_vertices_of_materials.size() == m_shader_memory->descriptor_set_count());

                for (size_t i = 0; i < m_vertices_of_materials.size(); ++i)
                {
                        if (m_vertices_of_materials[i].count <= 0)
                        {
                                continue;
                        }

                        vkCmdDraw(command_buffer, m_vertices_of_materials[i].count, 1, m_vertices_of_materials[i].offset, 0);
                }
        }
};

class Renderer final : public VulkanRenderer
{
        static constexpr mat4 SCALE = scale<double>(0.5, 0.5, 1);
        static constexpr mat4 TRANSLATE = translate<double>(1, 1, 0);
        const mat4 SCALE_BIAS_MATRIX = SCALE * TRANSLATE;

        const std::thread::id m_thread_id = std::this_thread::get_id();

        Color m_clear_color = Color(0);

        mat4 m_main_matrix = mat4(1);
        mat4 m_shadow_matrix = mat4(1);
        mat4 m_scale_bias_shadow_matrix = mat4(1);

        double m_shadow_zoom = 1;

        vulkan::VulkanInstance m_instance;
        vulkan::Sampler m_sampler;
        vulkan::Sampler m_shadow_sampler;
        vulkan::DescriptorSetLayout m_shared_descriptor_set_layout;
        vulkan::DescriptorSetLayout m_per_object_descriptor_set_layout;
        vulkan::DescriptorSetLayout m_shadow_descriptor_set_layout;
        shaders::SharedMemory m_shared_shader_memory;
        shaders::ShadowMemory m_shadow_shader_memory;
        vulkan::VertexShader m_vertex_shader;
        vulkan::GeometryShader m_geometry_shader;
        vulkan::FragmentShader m_fragment_shader;
        vulkan::VertexShader m_shadow_vertex_shader;
        vulkan::FragmentShader m_shadow_fragment_shader;
        std::vector<const vulkan::Shader*> m_shaders;
        std::vector<const vulkan::Shader*> m_shadow_shaders;

        std::unique_ptr<vulkan::SwapChain> m_swap_chain;

        DrawObjects<DrawObject> m_draw_objects;

        void set_light_a(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_light_a(light);
        }
        void set_light_d(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_light_d(light);
        }
        void set_light_s(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_light_s(light);
        }
        void set_background_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clear_color = color;
                create_command_buffers();
        }
        void set_default_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_default_color(color);
        }
        void set_wireframe_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_wireframe_color(color);
        }
        void set_default_ns(double default_ns) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_default_ns(default_ns);
        }
        void set_show_smooth(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_show_smooth(show);
        }
        void set_show_wireframe(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_show_wireframe(show);
        }
        void set_show_shadow(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_show_shadow(show);
                m_instance.set_draw_shadow(show);
        }
        void set_show_fog(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_materials(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_show_materials(show);
        }
        void set_shadow_zoom(double zoom) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shadow_zoom = zoom;

                create_swap_chain_and_command_buffers();
        }
        void set_matrices(const mat4& shadow_matrix, const mat4& main_matrix) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_main_matrix = main_matrix;
                m_shadow_matrix = shadow_matrix;
                m_scale_bias_shadow_matrix = SCALE_BIAS_MATRIX * shadow_matrix;

                set_matrices();
        }
        void set_light_direction(vec3 dir) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_direction_to_light(-to_vector<float>(dir));
        }
        void set_camera_direction(vec3 dir) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shared_shader_memory.set_direction_to_camera(-to_vector<float>(dir));
        }
        void set_size(int /*width*/, int /*height*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }

        void object_add(const Obj<3>* obj, double size, const vec3& position, int id, int scale_id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_draw_objects.add_object(std::make_unique<DrawObject>(m_instance, m_sampler, m_per_object_descriptor_set_layout,
                                                                       obj, size, position),
                                          id, scale_id);
                set_matrices();
        }
        void object_delete(int id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                bool delete_and_create_command_buffers = m_draw_objects.is_current_object(id);
                if (delete_and_create_command_buffers)
                {
                        delete_command_buffers();
                }
                m_draw_objects.delete_object(id);
                if (delete_and_create_command_buffers)
                {
                        create_command_buffers();
                }
                set_matrices();
        }
        void object_delete_all() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                bool delete_and_create_command_buffers = m_draw_objects.object() != nullptr;
                if (delete_and_create_command_buffers)
                {
                        delete_command_buffers();
                }
                m_draw_objects.delete_all();
                if (delete_and_create_command_buffers)
                {
                        create_command_buffers();
                }
                set_matrices();
        }
        void object_show(int id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_draw_objects.is_current_object(id))
                {
                        return;
                }
                const DrawObject* object = m_draw_objects.object();
                m_draw_objects.show_object(id);
                if (object != m_draw_objects.object())
                {
                        create_command_buffers();
                }
                set_matrices();
        }

        bool draw() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (!m_instance.draw_frame(*m_swap_chain))
                {
                        create_swap_chain_and_command_buffers();
                }

                return m_draw_objects.object() != nullptr;
        }

        void set_matrices()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_draw_objects.scale_object() || !m_draw_objects.object());

                if (m_draw_objects.scale_object())
                {
                        mat4 matrix = m_main_matrix * m_draw_objects.scale_object()->model_matrix();
                        mat4 scale_bias_shadow_matrix =
                                m_scale_bias_shadow_matrix * m_draw_objects.scale_object()->model_matrix();
                        mat4 shadow_matrix = m_shadow_matrix * m_draw_objects.scale_object()->model_matrix();

                        m_shared_shader_memory.set_matrices(matrix, scale_bias_shadow_matrix);
                        m_shadow_shader_memory.set_matrix(shadow_matrix);
                }
        }

        void create_swap_chain_and_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_shared_descriptor_set_layout != VK_NULL_HANDLE);
                ASSERT(m_per_object_descriptor_set_layout != VK_NULL_HANDLE);
                ASSERT(m_shaders.size() > 0);

                m_instance.device_wait_idle();

                //

                std::vector<VkDescriptorSetLayout> layouts(2);
                layouts[SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX] = m_shared_descriptor_set_layout;
                layouts[SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX] = m_per_object_descriptor_set_layout;

                std::vector<VkDescriptorSetLayout> shadow_layouts(1);
                shadow_layouts[0] = m_shadow_descriptor_set_layout;

                // Сначала надо удалить объект, а потом создавать
                m_swap_chain.reset();

                m_swap_chain = std::make_unique<vulkan::SwapChain>(m_instance.create_swap_chain(
                        PREFERRED_IMAGE_COUNT, REQUIRED_MINIMUM_SAMPLE_COUNT, m_shaders, shaders::Vertex::binding_descriptions(),
                        shaders::Vertex::attribute_descriptions(), layouts, m_shadow_shaders, shadow_layouts, m_shadow_zoom));

                m_shared_shader_memory.set_shadow_texture(m_shadow_sampler, m_swap_chain->shadow_texture());

                create_command_buffers(false /*wait_idle*/);
        }

        void draw_commands(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_shared_shader_memory.descriptor_set() != VK_NULL_HANDLE);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                std::array<VkDescriptorSet, 1> descriptor_sets = {m_shared_shader_memory.descriptor_set()};
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                        SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX, descriptor_sets.size(),
                                        descriptor_sets.data(), 0, nullptr);

                if (m_draw_objects.object())
                {
                        m_draw_objects.object()->draw_commands(pipeline_layout, command_buffer,
                                                               SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX);
                }
        }

        void shadow_draw_commands(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_shadow_shader_memory.descriptor_set() != VK_NULL_HANDLE);

                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                std::array<VkDescriptorSet, 1> descriptor_sets = {m_shadow_shader_memory.descriptor_set()};
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0 /*firstSet*/,
                                        descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

                if (m_draw_objects.object())
                {
                        m_draw_objects.object()->shadow_draw_commands(command_buffer);
                }
        }

        void create_command_buffers(bool wait_idle = true)
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_swap_chain);

                if (wait_idle)
                {
                        m_instance.device_wait_idle();
                }

                using std::placeholders::_1;
                using std::placeholders::_2;
                using std::placeholders::_3;

                m_swap_chain->create_command_buffers(m_clear_color, std::bind(&Renderer::draw_commands, this, _1, _2, _3),
                                                     std::bind(&Renderer::shadow_draw_commands, this, _1, _2, _3));
        }

        void delete_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_swap_chain);

                m_instance.device_wait_idle();
                m_swap_chain->delete_command_buffers();
        }

public:
        Renderer(const std::vector<std::string>& window_instance_extensions,
                 const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
                : m_instance(API_VERSION_MAJOR, API_VERSION_MINOR, instance_extensions() + window_instance_extensions,
                             device_extensions(), validation_layers(), required_features(), optional_features(), create_surface,
                             MAX_FRAMES_IN_FLIGHT),
                  m_sampler(vulkan::create_sampler(m_instance.device())),
                  m_shadow_sampler(vulkan::create_shadow_sampler(m_instance.device())),
                  m_shared_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                          m_instance.device(), shaders::SharedMemory::descriptor_set_layout_bindings())),
                  m_per_object_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                          m_instance.device(), shaders::PerObjectMemory::descriptor_set_layout_bindings())),
                  m_shadow_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                          m_instance.device(), shaders::ShadowMemory::descriptor_set_layout_bindings())),
                  m_shared_shader_memory(m_instance.device(), m_shared_descriptor_set_layout),
                  m_shadow_shader_memory(m_instance.device(), m_shadow_descriptor_set_layout),
                  m_vertex_shader(m_instance.device(), vertex_shader, "main"),
                  m_geometry_shader(m_instance.device(), geometry_shader, "main"),
                  m_fragment_shader(m_instance.device(), fragment_shader, "main"),
                  m_shadow_vertex_shader(m_instance.device(), shadow_vertex_shader, "main"),
                  m_shadow_fragment_shader(m_instance.device(), shadow_fragment_shader, "main"),
                  m_shaders({&m_vertex_shader, &m_geometry_shader, &m_fragment_shader}),
                  m_shadow_shaders({&m_shadow_vertex_shader, &m_shadow_fragment_shader})
        {
                create_swap_chain_and_command_buffers();

                LOG(vulkan::overview_physical_devices(m_instance.instance()));
        }

        ~Renderer() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                try
                {
                        m_instance.device_wait_idle();
                }
                catch (std::exception& e)
                {
                        LOG(std::string("Device wait idle exception in the Vulkan renderer destructor: ") + e.what());
                }
                catch (...)
                {
                        LOG("Device wait idle unknown exception in the Vulkan renderer destructor");
                }
        }
};
}

mat4 VulkanRenderer::ortho(double left, double right, double bottom, double top, double near, double far)
{
        return ortho_vulkan<double>(left, right, bottom, top, near, far);
}

std::unique_ptr<VulkanRenderer> create_vulkan_renderer(const std::vector<std::string>& window_instance_extensions,
                                                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
{
        return std::make_unique<Renderer>(window_instance_extensions, create_surface);
}
