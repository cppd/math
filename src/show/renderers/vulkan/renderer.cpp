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

#include <array>

constexpr int API_VERSION_MAJOR = 1;
constexpr int API_VERSION_MINOR = 0;

constexpr std::array<const char*, 0> INSTANCE_EXTENSIONS = {};
constexpr std::array<const char*, 0> DEVICE_EXTENSIONS = {};
constexpr std::array<const char*, 1> VALIDATION_LAYERS = {"VK_LAYER_LUNARG_standard_validation"};

using VERTEX_INDEX_TYPE = uint32_t;
constexpr VkIndexType VERTEX_INDEX_TYPE_VULKAN = VK_INDEX_TYPE_UINT32;

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
        vec2f texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions()
        {
                VkVertexInputBindingDescription binding_description = {};
                binding_description.binding = 0;
                binding_description.stride = sizeof(Vertex);
                binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return {binding_description};
        }

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions()
        {
                VkVertexInputAttributeDescription position_description = {};
                position_description.binding = 0;
                position_description.location = 0;
                position_description.format = VK_FORMAT_R32G32B32_SFLOAT;
                position_description.offset = offsetof(Vertex, position);

                VkVertexInputAttributeDescription texture_coordinates_description = {};
                texture_coordinates_description.binding = 0;
                texture_coordinates_description.location = 1;
                texture_coordinates_description.format = VK_FORMAT_R32G32_SFLOAT;
                texture_coordinates_description.offset = offsetof(Vertex, texture_coordinates);

                return {position_description, texture_coordinates_description};
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
        struct VertexShaderUniformBufferObject
        {
                Matrix<4, 4, float> mvp_matrix;
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
                        m_uniform_buffers.emplace_back(instance.device(), sizeof(VertexShaderUniformBufferObject));

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

        void set_uniform(const VertexShaderUniformBufferObject& ubo)
        {
                copy_to_buffer(0, Span(&ubo, sizeof(ubo)));
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

// clang-format off
constexpr std::array<Vertex, 8> vertices =
{
        Vertex{vec3f( 0.5, -0.5, 0.0), vec2f(1, 1)},
        Vertex{vec3f( 0.5,  0.5, 0.0), vec2f(1, 0)},
        Vertex{vec3f(-0.5,  0.5, 0.0), vec2f(0, 0)},
        Vertex{vec3f(-0.5, -0.5, 0.0), vec2f(0, 1)},

        Vertex{vec3f( 0.5, -0.5, 0.5), vec2f(1, 1)},
        Vertex{vec3f( 0.5,  0.5, 0.5), vec2f(1, 0)},
        Vertex{vec3f(-0.5,  0.5, 0.5), vec2f(0, 0)},
        Vertex{vec3f(-0.5, -0.5, 0.5), vec2f(0, 1)}
};
constexpr std::array<VERTEX_INDEX_TYPE, 12> vertex_indices =
{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
};
// clang-format on

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

class VulkanRendererImplementation final : public VulkanRenderer
{
        std::optional<vulkan::VulkanInstance> m_instance;

        std::optional<vulkan::VertexShader> m_vertex_shader;
        std::optional<vulkan::FragmentShader> m_fragment_shader;

        std::optional<ShaderMemory> m_shader_memory;

        uint32_t m_vertex_count;
        std::optional<vulkan::VertexBufferWithDeviceLocalMemory> m_vertex_buffer;
        std::optional<vulkan::IndexBufferWithDeviceLocalMemory> m_vertex_index_buffer;
        Color m_clear_color = Color(0);

        std::optional<vulkan::SwapChain> m_swap_chain;

        void set_light_a(const Color& /*light*/) override
        {
        }
        void set_light_d(const Color& /*light*/) override
        {
        }
        void set_light_s(const Color& /*light*/) override
        {
        }
        void set_background_color(const Color& color) override
        {
                m_clear_color = color;
                create_command_buffers();
        }
        void set_default_color(const Color& /*color*/) override
        {
        }
        void set_wireframe_color(const Color& /*color*/) override
        {
        }
        void set_default_ns(double /*default_ns*/) override
        {
        }
        void set_show_smooth(bool /*show*/) override
        {
        }
        void set_show_wireframe(bool /*show*/) override
        {
        }
        void set_show_shadow(bool /*show*/) override
        {
        }
        void set_show_fog(bool /*show*/) override
        {
        }
        void set_show_materials(bool /*show*/) override
        {
        }
        void set_shadow_zoom(double /*zoom*/) override
        {
        }
        void set_matrices(const mat4& /*shadow_matrix*/, const mat4& main_matrix) override
        {
                ShaderMemory::VertexShaderUniformBufferObject ubo;
                ubo.mvp_matrix = transpose(to_matrix<float>(main_matrix));
                m_shader_memory->set_uniform(ubo);
        }
        void set_light_direction(vec3 /*dir*/) override
        {
        }
        void set_camera_direction(vec3 /*dir*/) override
        {
        }
        void set_size(int /*width*/, int /*height*/) override
        {
        }

        void object_add(const Obj<3>* /*obj*/, double /*size*/, const vec3& /*position*/, int /*id*/, int /*scale_id*/) override
        {
        }
        void object_delete(int /*id*/) override
        {
        }
        void object_show(int /*id*/) override
        {
        }
        void object_delete_all() override
        {
        }

        bool draw() override
        {
                update_uniforms(*m_shader_memory);

                if (!m_instance->draw_frame(*m_swap_chain))
                {
                        create_swap_chain_and_command_buffers();
                }

                return true;
        }

        void create_swap_chain_and_command_buffers()
        {
                m_instance->device_wait_idle();

                ASSERT(m_vertex_shader);
                ASSERT(m_fragment_shader);
                ASSERT(m_shader_memory);

                m_instance->create_swap_chain(*m_vertex_shader, *m_fragment_shader, Vertex::binding_descriptions(),
                                              Vertex::attribute_descriptions(), m_shader_memory->descriptor_set_layout(),
                                              &m_swap_chain);

                create_command_buffers(false /*wait_idle*/);
        }

        void create_command_buffers(bool wait_idle = true)
        {
                if (wait_idle)
                {
                        m_instance->device_wait_idle();
                }

                ASSERT(m_swap_chain);
                ASSERT(m_vertex_buffer);
                ASSERT(m_vertex_index_buffer);
                ASSERT(m_shader_memory);

                m_swap_chain->create_command_buffers(*m_vertex_buffer, *m_vertex_index_buffer, VERTEX_INDEX_TYPE_VULKAN,
                                                     m_vertex_count, m_clear_color, m_shader_memory->descriptor_set());
        }

public:
        VulkanRendererImplementation(const std::vector<std::string>& window_instance_extensions,
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

                m_vertex_count = vertex_indices.size();
                m_vertex_buffer.emplace(m_instance->create_vertex_buffer(vertices.size() * sizeof(vertices[0]), vertices.data()));
                m_vertex_index_buffer.emplace(m_instance->create_index_buffer(vertex_indices.size() * sizeof(vertex_indices[0]),
                                                                              vertex_indices.data()));

                create_swap_chain_and_command_buffers();

                LOG(vulkan_overview_physical_devices_for_log(m_instance->instance()));
        }

        ~VulkanRendererImplementation() override
        {
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
        return std::make_unique<VulkanRendererImplementation>(window_instance_extensions, create_surface);
}
