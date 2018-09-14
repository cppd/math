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

class ShaderMemory
{
        vulkan::Sampler m_sampler;
        std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        std::vector<vulkan::Texture> m_textures;
        vulkan::Descriptors m_descriptors;

        void copy_to_buffer(uint32_t index, const Span<const void>& data) const
        {
                ASSERT(index < m_uniform_buffers.size());
                ASSERT(data.size() == m_uniform_buffers[index].size());

                m_uniform_buffers[index].copy(data.data());
        }

public:
        struct VertexShaderUniformMatrices
        {
                Matrix<4, 4, float> mvp;

                VertexShaderUniformMatrices(const mat4& mvp_) : mvp(transpose(to_matrix<float>(mvp_)))
                {
                }
        };

        struct FragmentShaderUniformBufferObject0
        {
                float value_r;
                float value_g;
        };

        struct FragmentShaderUniformBufferObject1
        {
                float value_b;
        };

        ShaderMemory(const vulkan::VulkanInstance& instance)
        {
                m_sampler = vulkan::create_sampler(instance.device());

                std::vector<VkDescriptorBufferInfo> buffer_infos;
                std::vector<VkDescriptorImageInfo> image_infos;
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                {
                        m_uniform_buffers.emplace_back(instance.device(), sizeof(VertexShaderUniformMatrices));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        buffer_infos.push_back(buffer_info);

                        VkDescriptorSetLayoutBinding binding = {};
                        binding.binding = 0;
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        binding.descriptorCount = 1;
                        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                        bindings.push_back(binding);
                }
                {
                        m_uniform_buffers.emplace_back(instance.device(), sizeof(FragmentShaderUniformBufferObject0));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        buffer_infos.push_back(buffer_info);

                        VkDescriptorSetLayoutBinding binding = {};
                        binding.binding = 1;
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        binding.descriptorCount = 1;
                        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                        bindings.push_back(binding);
                }
                {
                        m_uniform_buffers.emplace_back(instance.device(), sizeof(FragmentShaderUniformBufferObject1));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        buffer_infos.push_back(buffer_info);

                        VkDescriptorSetLayoutBinding binding = {};
                        binding.binding = 2;
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        binding.descriptorCount = 1;
                        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                        bindings.push_back(binding);
                }
                {
                        m_textures.push_back(
                                instance.create_texture(2, 2,
                                                        std::vector<unsigned char>({255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255,
                                                                                    255, 255, 255, 255, 255})));

                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = m_textures.back().image_layout();
                        image_info.imageView = m_textures.back().image_view();
                        image_info.sampler = m_sampler;

                        image_infos.push_back(image_info);

                        VkDescriptorSetLayoutBinding binding = {};
                        binding.binding = 3;
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        binding.descriptorCount = 1;
                        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                        binding.pImmutableSamplers = nullptr;

                        bindings.push_back(binding);
                }

                m_descriptors = vulkan::Descriptors(instance.device(), bindings, buffer_infos, image_infos);
        }

        VkDescriptorSetLayout descriptor_set_layout() const noexcept
        {
                return m_descriptors.descriptor_set_layout();
        }

        VkDescriptorSet descriptor_set() const noexcept
        {
                return m_descriptors.descriptor_set();
        }

        void set_uniform(const VertexShaderUniformMatrices& matrices)
        {
                copy_to_buffer(0, Span(&matrices, sizeof(matrices)));
        }

        void set_uniform(const FragmentShaderUniformBufferObject0& ubo)
        {
                copy_to_buffer(1, Span(&ubo, sizeof(ubo)));
        }

        void set_uniform(const FragmentShaderUniformBufferObject1& ubo)
        {
                copy_to_buffer(2, Span(&ubo, sizeof(ubo)));
        }
};

//

void update_uniforms(ShaderMemory& shader_memory)
{
        const double radians = time_in_seconds() * 2 * PI<double>;

        ShaderMemory::FragmentShaderUniformBufferObject0 ubo0;
        ubo0.value_r = 0.5 * (1 + std::sin(radians));
        ubo0.value_g = 0.5 * (1 + std::sin(radians * 2));
        shader_memory.set_uniform(ubo0);

        ShaderMemory::FragmentShaderUniformBufferObject1 ubo1;
        ubo1.value_b = 0.5 * (1 + std::sin(radians * 4));
        shader_memory.set_uniform(ubo1);
}

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

        mat4 m_model_matrix;

        DrawType m_draw_type;

public:
        DrawObject(const vulkan::VulkanInstance& instance, const Obj<3>* obj, double size, const vec3& position)
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
        uint32_t vertex_count() const noexcept
        {
                return m_vertex_count;
        }
        VkBuffer vertex_buffer() const noexcept
        {
                return m_vertex_buffer ? static_cast<VkBuffer>(*m_vertex_buffer) : VK_NULL_HANDLE;
        }
        VkBuffer index_buffer() const noexcept
        {
                return m_vertex_index_buffer ? static_cast<VkBuffer>(*m_vertex_index_buffer) : VK_NULL_HANDLE;
        }
        const mat4& model_matrix() const noexcept
        {
                return m_model_matrix;
        }
};

class Renderer final : public VulkanRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::optional<vulkan::VulkanInstance> m_instance;

        std::optional<vulkan::VertexShader> m_vertex_shader;
        std::optional<vulkan::FragmentShader> m_fragment_shader;

        std::optional<ShaderMemory> m_shader_memory;

        Color m_clear_color = Color(0);

        mat4 m_main_matrix = mat4(1);

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

                m_draw_objects.add_object(std::make_unique<DrawObject>(*m_instance, obj, size, position), id, scale_id);
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

                update_uniforms(*m_shader_memory);

                if (!m_instance->draw_frame(*m_swap_chain))
                {
                        create_swap_chain_and_command_buffers();
                }

                return m_draw_objects.object() && m_draw_objects.object()->vertex_buffer() != VK_NULL_HANDLE;
        }

        void set_matrices()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_draw_objects.scale_object() || !m_draw_objects.object());

                if (m_draw_objects.scale_object())
                {
                        mat4 mvp = m_main_matrix * m_draw_objects.scale_object()->model_matrix();
                        m_shader_memory->set_uniform(ShaderMemory::VertexShaderUniformMatrices(mvp));
                }
        }

        void create_swap_chain_and_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                ASSERT(m_vertex_shader);
                ASSERT(m_fragment_shader);
                ASSERT(m_shader_memory);

                m_instance->device_wait_idle();

                m_instance->create_swap_chain(*m_vertex_shader, *m_fragment_shader, Vertex::binding_descriptions(),
                                              Vertex::attribute_descriptions(), m_shader_memory->descriptor_set_layout(),
                                              &m_swap_chain);

                create_command_buffers(false /*wait_idle*/);
        }

        void commands(VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkCommandBuffer command_buffer)
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (!m_draw_objects.object() || m_draw_objects.object()->vertex_buffer() == VK_NULL_HANDLE)
                {
                        return;
                }

                //

                if (m_draw_objects.object()->vertex_count() <= 0 || (m_draw_objects.object()->vertex_count() % 3 != 0))
                {
                        error("Error vertex count (" + to_string(m_draw_objects.object()->vertex_count()) +
                              ") for triangle list primitive topology");
                }
                ASSERT(m_draw_objects.object()->vertex_buffer() != VK_NULL_HANDLE);
                ASSERT(m_shader_memory);
                ASSERT(m_shader_memory->descriptor_set() != VK_NULL_HANDLE);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                std::array<VkDescriptorSet, 1> descriptor_sets = {m_shader_memory->descriptor_set()};
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0,
                                        descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

                std::array<VkBuffer, 1> vertex_buffers = {m_draw_objects.object()->vertex_buffer()};
                std::array<VkDeviceSize, 1> offsets = {0};
                static_assert(vertex_buffers.size() == offsets.size());
                vkCmdBindVertexBuffers(command_buffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

                if (m_draw_objects.object()->index_buffer() == VK_NULL_HANDLE)
                {
                        vkCmdDraw(command_buffer, m_draw_objects.object()->vertex_count(), 1, 0, 0);
                }
                else
                {
                        vkCmdBindIndexBuffer(command_buffer, m_draw_objects.object()->index_buffer(), 0,
                                             VULKAN_VERTEX_INDEX_TYPE);
                        vkCmdDrawIndexed(command_buffer, m_draw_objects.object()->vertex_count(), 1, 0, 0, 0);
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

                m_swap_chain->create_command_buffers(m_clear_color, std::bind(&Renderer::commands, this, _1, _2, _3));
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

                m_vertex_shader.emplace(m_instance->device(), vertex_shader, "main");
                m_fragment_shader.emplace(m_instance->device(), fragment_shader, "main");

                m_shader_memory.emplace(m_instance.value());

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
