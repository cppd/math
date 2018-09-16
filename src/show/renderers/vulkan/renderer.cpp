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

#include "com/log.h"
#include "com/mat_alg.h"
#include "com/math.h"
#include "com/time.h"
#include "com/vec.h"
#include "graphics/vulkan/common.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/query.h"
#include "obj/obj_alg.h"
#include "show/renderers/com.h"

#include <array>
#include <thread>

constexpr int API_VERSION_MAJOR = 1;
constexpr int API_VERSION_MINOR = 0;

constexpr std::array<const char*, 0> INSTANCE_EXTENSIONS = {};
constexpr std::array<const char*, 0> DEVICE_EXTENSIONS = {};
constexpr std::array<const char*, 1> VALIDATION_LAYERS = {"VK_LAYER_LUNARG_standard_validation"};

constexpr VkIndexType VULKAN_VERTEX_INDEX_TYPE = VK_INDEX_TYPE_UINT32;
using VERTEX_INDEX_TYPE =
        std::conditional_t<VULKAN_VERTEX_INDEX_TYPE == VK_INDEX_TYPE_UINT32, uint32_t,
                           std::conditional_t<VULKAN_VERTEX_INDEX_TYPE == VK_INDEX_TYPE_UINT16, uint16_t, void>>;

constexpr uint32_t SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX = 0;
constexpr uint32_t SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX = 1;

// clang-format off
constexpr uint32_t vertex_shader[]
{
#include "renderer_triangles.vert.spr"
};
constexpr uint32_t fragment_shader[]
{
#include "renderer_triangles.frag.spr"
};
// clang-format on

namespace
{
constexpr const char LOG_MESSAGE_BEGIN[] = "\n---Vulkan---\n";
constexpr const char LOG_MESSAGE_END[] = "\n---";

std::string vulkan_overview_for_log(const std::vector<std::string>& window_instance_extensions)
{
        std::string extensions = "Required Window Extensions";
        if (window_instance_extensions.size() > 0)
        {
                for (const std::string& s : window_instance_extensions)
                {
                        extensions += "\n  " + s;
                }
        }
        else
        {
                extensions += "\n  no extensions";
        }

        return LOG_MESSAGE_BEGIN + vulkan::overview() + LOG_MESSAGE_END + LOG_MESSAGE_BEGIN + extensions + LOG_MESSAGE_END;
}

std::string vulkan_overview_physical_devices_for_log(VkInstance instance)
{
        return LOG_MESSAGE_BEGIN + vulkan::overview_physical_devices(instance) + LOG_MESSAGE_END;
}

struct Vertex
{
        vec3f position;
        vec3f normal;
        vec2f texture_coordinates;

        constexpr Vertex(const vec3f& position_, const vec3f& normal_, const vec2f& texture_coordinates_)
                : position(position_), normal(normal_), texture_coordinates(texture_coordinates_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions()
        {
                std::vector<VkVertexInputBindingDescription> descriptions;

                {
                        VkVertexInputBindingDescription d = {};
                        d.binding = 0;
                        d.stride = sizeof(Vertex);
                        d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                        descriptions.push_back(d);
                }

                return descriptions;
        }

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions()
        {
                std::vector<VkVertexInputAttributeDescription> descriptions;

                {
                        VkVertexInputAttributeDescription d = {};
                        d.binding = 0;
                        d.location = 0;
                        d.format = VK_FORMAT_R32G32B32_SFLOAT;
                        d.offset = offsetof(Vertex, position);

                        descriptions.push_back(d);
                }
                {
                        VkVertexInputAttributeDescription d = {};
                        d.binding = 0;
                        d.location = 1;
                        d.format = VK_FORMAT_R32G32B32_SFLOAT;
                        d.offset = offsetof(Vertex, normal);

                        descriptions.push_back(d);
                }
                {
                        VkVertexInputAttributeDescription d = {};
                        d.binding = 0;
                        d.location = 2;
                        d.format = VK_FORMAT_R32G32_SFLOAT;
                        d.offset = offsetof(Vertex, texture_coordinates);

                        descriptions.push_back(d);
                }

                return descriptions;
        }
};

//

template <typename T>
void copy_to_buffer(const std::vector<vulkan::UniformBufferWithHostVisibleMemory>& buffers, uint32_t index, const T& data)
{
        ASSERT(index < buffers.size());
        ASSERT(sizeof(data) == buffers[index].size());

        buffers[index].copy(&data);
}

template <typename T>
void copy_to_buffer(const vulkan::UniformBufferWithHostVisibleMemory& buffer, const T& data)
{
        ASSERT(sizeof(data) == buffer.size());

        buffer.copy(&data);
}

class SharedShaderMemory
{
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        vulkan::DescriptorSet m_descriptor_set;

public:
        static std::vector<VkDescriptorSetLayoutBinding> shared_descriptor_set_layout_bindings()
        {
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                {
                        VkDescriptorSetLayoutBinding b = {};
                        b.binding = 0;
                        b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        b.descriptorCount = 1;
                        b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                        bindings.push_back(b);
                }

                return bindings;
        }

        struct VertexShaderUniformMatrices
        {
                Matrix<4, 4, float> mvp;

                VertexShaderUniformMatrices(const mat4& mvp_) : mvp(transpose(to_matrix<float>(mvp_)))
                {
                }
        };

        //

        SharedShaderMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
                : m_descriptors(vulkan::Descriptors(device, 1, descriptor_set_layout, shared_descriptor_set_layout_bindings()))
        {
                std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;

                {
                        m_uniform_buffers.emplace_back(device, sizeof(VertexShaderUniformMatrices));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        infos.push_back(buffer_info);
                }

                m_descriptor_set = m_descriptors.create_descriptor_set(infos);
        }

        SharedShaderMemory(const SharedShaderMemory&) = delete;
        SharedShaderMemory& operator=(const SharedShaderMemory&) = delete;
        SharedShaderMemory& operator=(SharedShaderMemory&&) = delete;

        SharedShaderMemory(SharedShaderMemory&&) = default;
        ~SharedShaderMemory() = default;

        //

        VkDescriptorSet descriptor_set() const noexcept
        {
                return m_descriptor_set;
        }

        void set_uniform(const VertexShaderUniformMatrices& matrices) const
        {
                copy_to_buffer(m_uniform_buffers, 0, matrices);
        }
};

class PerObjectShaderMemory
{
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        std::vector<vulkan::DescriptorSet> m_descriptor_sets;

public:
        static std::vector<VkDescriptorSetLayoutBinding> per_object_descriptor_set_layout_bindings()
        {
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                {
                        VkDescriptorSetLayoutBinding b = {};
                        b.binding = 0;
                        b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        b.descriptorCount = 1;
                        b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                        bindings.push_back(b);
                }
                {
                        VkDescriptorSetLayoutBinding b = {};
                        b.binding = 1;
                        b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        b.descriptorCount = 1;
                        b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                        b.pImmutableSamplers = nullptr;

                        bindings.push_back(b);
                }

                return bindings;
        }

        struct FragmentShaderUniformBufferObject
        {
                float value_r;
                float value_g;
                float value_b;
        };

        //

        PerObjectShaderMemory(const vulkan::Device& device, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                              const std::vector<FragmentShaderUniformBufferObject>& uniforms,
                              const std::vector<vulkan::Texture>& textures)
                : m_descriptors(vulkan::Descriptors(device, uniforms.size(), descriptor_set_layout,
                                                    per_object_descriptor_set_layout_bindings()))
        {
                ASSERT(uniforms.size() > 0);
                ASSERT(uniforms.size() == textures.size());

                std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;

                for (size_t i = 0; i < uniforms.size(); ++i)
                {
                        infos.clear();
                        {
                                m_uniform_buffers.emplace_back(device, sizeof(FragmentShaderUniformBufferObject));

                                VkDescriptorBufferInfo buffer_info = {};
                                buffer_info.buffer = m_uniform_buffers.back();
                                buffer_info.offset = 0;
                                buffer_info.range = m_uniform_buffers.back().size();

                                infos.push_back(buffer_info);
                        }
                        {
                                VkDescriptorImageInfo image_info = {};
                                image_info.imageLayout = textures[i].image_layout();
                                image_info.imageView = textures[i].image_view();
                                image_info.sampler = sampler;

                                infos.push_back(image_info);
                        }
                        m_descriptor_sets.push_back(m_descriptors.create_descriptor_set(infos));
                }

                ASSERT(uniforms.size() == m_descriptor_sets.size());

                for (size_t i = 0; i < uniforms.size(); ++i)
                {
                        copy_to_buffer(m_uniform_buffers[i], uniforms[i]);
                }
        }

        PerObjectShaderMemory(const PerObjectShaderMemory&) = delete;
        PerObjectShaderMemory& operator=(const PerObjectShaderMemory&) = delete;
        PerObjectShaderMemory& operator=(PerObjectShaderMemory&&) = delete;

        PerObjectShaderMemory(PerObjectShaderMemory&&) = default;
        ~PerObjectShaderMemory() = default;

        //

        VkDescriptorSet descriptor_set(uint32_t index) const noexcept
        {
                ASSERT(index < m_descriptor_sets.size());

                return m_descriptor_sets[index];
        }
};

//

std::vector<Vertex> load_face_vertices(const Obj<3>& obj)
{
        const std::vector<vec3f>& obj_vertices = obj.vertices();
        const std::vector<vec3f>& obj_normals = obj.normals();
        const std::vector<vec2f>& obj_texcoords = obj.texcoords();

        std::vector<Vertex> vertices;
        vertices.reserve(obj.facets().size() * 3);

        vec3f v0, v1, v2, n0, n1, n2;
        vec2f t0, t1, t2;

        for (const Obj<3>::Facet& f : obj.facets())
        {
                v0 = obj_vertices[f.vertices[0]];
                v1 = obj_vertices[f.vertices[1]];
                v2 = obj_vertices[f.vertices[2]];

                if (f.has_normal)
                {
                        n0 = obj_normals[f.normals[0]];
                        n1 = obj_normals[f.normals[1]];
                        n2 = obj_normals[f.normals[2]];
                }
                else
                {
#if 0
                        n0 = n1 = n2 = vec3f(0);
#else
                        n0 = n1 = n2 = normalize(cross(v1 - v0, v2 - v0));
#endif
                }

                if (f.has_texcoord)
                {
                        t0 = obj_texcoords[f.texcoords[0]];
                        t1 = obj_texcoords[f.texcoords[1]];
                        t2 = obj_texcoords[f.texcoords[2]];
                }
                else
                {
#if 0
                        t0 = t1 = t2 = vec2f(0);
#else
                        t0 = vec2f(0, 0);
                        t1 = vec2f(1, 0);
                        t2 = vec2f(0, 1);
#endif
                }

                // vertices.emplace_back(v0, n0, t0, f.material, f.has_texcoord, f.has_normal);
                // vertices.emplace_back(v1, n1, t1, f.material, f.has_texcoord, f.has_normal);
                // vertices.emplace_back(v2, n2, t2, f.material, f.has_texcoord, f.has_normal);
                vertices.emplace_back(v0, n0, t0);
                vertices.emplace_back(v1, n1, t1);
                vertices.emplace_back(v2, n2, t2);
        }

        return vertices;
}

template <unsigned i>
std::vector<unsigned char> default_pixels_2x2();
template <>
std::vector<unsigned char> default_pixels_2x2<0>()
{
        // clang-format off
        return
        {
                255,   0,   0, 255,
                  0, 255,   0, 255,
                  0,   0, 255, 255,
                255, 255, 255, 255
        };
        // clang-format on
}
template <>
std::vector<unsigned char> default_pixels_2x2<1>()
{
        // clang-format off
        return
        {
                  0,   0, 255, 255,
                  0, 255,   0, 255,
                255,   0,   0, 255,
                255, 255, 255, 255
        };
        // clang-format on
}
template <>
std::vector<unsigned char> default_pixels_2x2<2>()
{
        // clang-format off
        return
        {
                255,   0, 255, 255,
                  0, 255,   0, 255,
                255,   0, 255, 255,
                255, 255, 255, 255
        };
        // clang-format on
}

class DrawObject
{
#if 0
        // clang-format off
        static constexpr std::array<Vertex, 8> VERTICES =
        {
                Vertex(vec3f( 0.5, -0.5, 0.0), vec3f(0, 0, 1), vec2f(1, 1)),
                Vertex(vec3f( 0.5,  0.5, 0.0), vec3f(0, 0, 1), vec2f(1, 0)),
                Vertex(vec3f(-0.5,  0.5, 0.0), vec3f(0, 0, 1), vec2f(0, 0)),
                Vertex(vec3f(-0.5, -0.5, 0.0), vec3f(0, 0, 1), vec2f(0, 1)),

                Vertex(vec3f( 0.5, -0.5, 0.5), vec3f(0, 0, 1), vec2f(1, 1)),
                Vertex(vec3f( 0.5,  0.5, 0.5), vec3f(0, 0, 1), vec2f(1, 0)),
                Vertex(vec3f(-0.5,  0.5, 0.5), vec3f(0, 0, 1), vec2f(0, 0)),
                Vertex(vec3f(-0.5, -0.5, 0.5), vec3f(0, 0, 1), vec2f(0, 1))
        };

        static constexpr std::array<VERTEX_INDEX_TYPE, 12> VERTEX_INDICES =
        {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4
        };
        // clang-format on
#endif

        uint32_t m_vertex_count;
        std::optional<vulkan::VertexBufferWithDeviceLocalMemory> m_vertex_buffer;
        std::optional<vulkan::IndexBufferWithDeviceLocalMemory> m_vertex_index_buffer;

        std::vector<vulkan::Texture> m_textures;
        std::optional<PerObjectShaderMemory> m_shader_memory;

        mat4 m_model_matrix;
        DrawType m_draw_type;

public:
        DrawObject(const vulkan::VulkanInstance& instance, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                   const Obj<3>* obj, double size, const vec3& position)
                : m_model_matrix(model_vertex_matrix(obj, size, position)), m_draw_type(draw_type_of_obj(obj))
        {
                if (m_draw_type == DrawType::Triangles)
                {
                        std::vector<Vertex> vertices = load_face_vertices(*obj);
                        if (vertices.size() == 0)
                        {
                                m_vertex_count = 0;
                                return;
                        }

                        m_vertex_count = vertices.size();
                        m_vertex_buffer.emplace(instance.create_vertex_buffer(vertices));

                        ASSERT((m_vertex_count > 0) && (m_vertex_count % 3 == 0));

                        //

                        std::vector<PerObjectShaderMemory::FragmentShaderUniformBufferObject> uniforms;

                        m_textures.emplace_back(instance.create_texture(2, 2, default_pixels_2x2<0>()));
                        uniforms.push_back({1.0, 1.0, 1.0});

                        m_textures.emplace_back(instance.create_texture(2, 2, default_pixels_2x2<1>()));
                        uniforms.push_back({1.0, 1.0, 1.0});

                        m_textures.emplace_back(instance.create_texture(2, 2, default_pixels_2x2<2>()));
                        uniforms.push_back({1.0, 1.0, 1.0});

                        m_shader_memory.emplace(instance.device(), sampler, descriptor_set_layout, uniforms, m_textures);
                }
                else
                {
#if 1
                        m_vertex_count = 0;
#else

                        m_model_matrix = mat4(1);
                        m_vertex_count = VERTEX_INDICES.size();
                        m_vertex_buffer.emplace(instance.create_vertex_buffer(VERTICES));
                        m_vertex_index_buffer.emplace(instance.create_index_buffer(VERTEX_INDICES));
#endif
                }
        }

        const mat4& model_matrix() const noexcept
        {
                return m_model_matrix;
        }

        void draw_commands(VkPipelineLayout pipeline_layout, VkCommandBuffer command_buffer,
                           uint32_t description_set_layout_index) const
        {
                if (!m_vertex_buffer.has_value())
                {
                        return;
                }

                std::array<VkDescriptorSet, 1> descriptor_sets = {m_shader_memory->descriptor_set(1)};
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                        description_set_layout_index, descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

                std::array<VkBuffer, 1> vertex_buffers = {*m_vertex_buffer};
                std::array<VkDeviceSize, vertex_buffers.size()> offsets = {0};
                vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

                if (!m_vertex_index_buffer.has_value())
                {
                        vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
                }
                else
                {
                        vkCmdBindIndexBuffer(command_buffer, *m_vertex_index_buffer, 0, VULKAN_VERTEX_INDEX_TYPE);
                        vkCmdDrawIndexed(command_buffer, m_vertex_count, 1, 0, 0, 0);
                }
        }

#if 0
        void update() const
        {
                const double radians = time_in_seconds() * 2 * PI<double>;

                PerObjectShaderMemory::FragmentShaderUniformBufferObject ubo;
                ubo.value_r = 0.5 * (1 + std::sin(radians));
                ubo.value_g = 0.5 * (1 + std::sin(radians * 2));
                ubo.value_b = 0.5 * (1 + std::sin(radians * 4));
                m_shader_memory.set_uniform(ubo);
        }
#endif
};

class Renderer final : public VulkanRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        Color m_clear_color = Color(0);
        mat4 m_main_matrix = mat4(1);

        std::optional<vulkan::VulkanInstance> m_instance;
        vulkan::Sampler m_sampler;
        vulkan::DescriptorSetLayout m_shared_descriptor_set_layout;
        vulkan::DescriptorSetLayout m_per_object_descriptor_set_layout;
        std::optional<SharedShaderMemory> m_shared_shader_memory;
        std::optional<vulkan::VertexShader> m_vertex_shader;
        std::optional<vulkan::FragmentShader> m_fragment_shader;
        std::optional<vulkan::SwapChain> m_swap_chain;

        DrawObjects<DrawObject> m_draw_objects;

        void set_light_a(const Color& /*light*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_light_d(const Color& /*light*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_light_s(const Color& /*light*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_background_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clear_color = color;
                create_command_buffers();
        }
        void set_default_color(const Color& /*color*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_wireframe_color(const Color& /*color*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_default_ns(double /*default_ns*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_smooth(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_wireframe(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_shadow(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_fog(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_show_materials(bool /*show*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_shadow_zoom(double /*zoom*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_matrices(const mat4& /*shadow_matrix*/, const mat4& main_matrix) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_main_matrix = main_matrix;
                set_matrices();
        }
        void set_light_direction(vec3 /*dir*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_camera_direction(vec3 /*dir*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }
        void set_size(int /*width*/, int /*height*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());
        }

        void object_add(const Obj<3>* obj, double size, const vec3& position, int id, int scale_id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_draw_objects.add_object(std::make_unique<DrawObject>(*m_instance, m_sampler, m_per_object_descriptor_set_layout,
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

                if (!m_instance->draw_frame(*m_swap_chain))
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
                        mat4 mvp = m_main_matrix * m_draw_objects.scale_object()->model_matrix();
                        m_shared_shader_memory->set_uniform(SharedShaderMemory::VertexShaderUniformMatrices(mvp));
                }
        }

        void create_swap_chain_and_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_vertex_shader);
                ASSERT(m_fragment_shader);
                ASSERT(m_shared_shader_memory);
                ASSERT(m_shared_descriptor_set_layout != VK_NULL_HANDLE);
                ASSERT(m_per_object_descriptor_set_layout != VK_NULL_HANDLE);

                m_instance->device_wait_idle();

                std::vector<VkDescriptorSetLayout> layouts(2);
                layouts[SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX] = m_shared_descriptor_set_layout;
                layouts[SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX] = m_per_object_descriptor_set_layout;
                // Сначала надо удалить объект, а потом создавать
                m_swap_chain.reset();
                m_swap_chain.emplace(m_instance->create_swap_chain(*m_vertex_shader, *m_fragment_shader,
                                                                   Vertex::binding_descriptions(),
                                                                   Vertex::attribute_descriptions(), layouts));

                create_command_buffers(false /*wait_idle*/);
        }

        void draw_commands(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer)
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_shared_shader_memory);
                ASSERT(m_shared_shader_memory->descriptor_set() != VK_NULL_HANDLE);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                std::array<VkDescriptorSet, 1> descriptor_sets = {m_shared_shader_memory->descriptor_set()};
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                        SHADER_SHARED_DESCRIPTION_SET_LAYOUT_INDEX, descriptor_sets.size(),
                                        descriptor_sets.data(), 0, nullptr);

                if (m_draw_objects.object())
                {
                        m_draw_objects.object()->draw_commands(pipeline_layout, command_buffer,
                                                               SHADER_PER_OBJECT_DESCRIPTION_SET_LAYOUT_INDEX);
                }
        }

        void create_command_buffers(bool wait_idle = true)
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_swap_chain);

                if (wait_idle)
                {
                        m_instance->device_wait_idle();
                }

                using std::placeholders::_1;
                using std::placeholders::_2;
                using std::placeholders::_3;

                m_swap_chain->create_command_buffers(m_clear_color, std::bind(&Renderer::draw_commands, this, _1, _2, _3));
        }

        void delete_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_swap_chain);

                m_instance->device_wait_idle();
                m_swap_chain->delete_command_buffers();
        }

public:
        Renderer(const std::vector<std::string>& window_instance_extensions,
                 const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        {
                LOG(vulkan_overview_for_log(window_instance_extensions));

                std::vector<std::string> instance_extensions(std::cbegin(INSTANCE_EXTENSIONS), std::cend(INSTANCE_EXTENSIONS));
                std::vector<std::string> device_extensions(std::cbegin(DEVICE_EXTENSIONS), std::cend(DEVICE_EXTENSIONS));
                std::vector<std::string> validation_layers(std::cbegin(VALIDATION_LAYERS), std::cend(VALIDATION_LAYERS));

                m_instance.emplace(API_VERSION_MAJOR, API_VERSION_MINOR, instance_extensions + window_instance_extensions,
                                   device_extensions, validation_layers, create_surface);

                m_sampler = vulkan::create_sampler(m_instance->device());

                m_shared_descriptor_set_layout = vulkan::create_descriptor_set_layout(
                        m_instance->device(), SharedShaderMemory::shared_descriptor_set_layout_bindings());

                m_per_object_descriptor_set_layout = vulkan::create_descriptor_set_layout(
                        m_instance->device(), PerObjectShaderMemory::per_object_descriptor_set_layout_bindings());

                m_shared_shader_memory.emplace(m_instance->device(), m_shared_descriptor_set_layout);

                m_vertex_shader.emplace(m_instance->device(), vertex_shader, "main");
                m_fragment_shader.emplace(m_instance->device(), fragment_shader, "main");

                create_swap_chain_and_command_buffers();

                LOG(vulkan_overview_physical_devices_for_log(m_instance->instance()));
        }

        ~Renderer() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_instance->device_wait_idle();
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
