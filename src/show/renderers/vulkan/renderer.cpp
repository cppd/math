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
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/query.h"

#include <array>

constexpr int API_VERSION_MAJOR = 1;
constexpr int API_VERSION_MINOR = 0;

constexpr std::array<const char*, 0> INSTANCE_EXTENSIONS = {};
constexpr std::array<const char*, 0> DEVICE_EXTENSIONS = {};
constexpr std::array<const char*, 1> VALIDATION_LAYERS = {"VK_LAYER_LUNARG_standard_validation"};

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
        vec2f position;
        vec3f color;

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
                position_description.format = VK_FORMAT_R32G32_SFLOAT;
                position_description.offset = offsetof(Vertex, position);

                VkVertexInputAttributeDescription color_description = {};
                color_description.binding = 0;
                color_description.location = 1;
                color_description.format = VK_FORMAT_R32G32B32_SFLOAT;
                color_description.offset = offsetof(Vertex, color);

                return {position_description, color_description};
        }
};

//

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

std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = 0;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                // layout_binding.pImmutableSamplers = nullptr;

                bindings.push_back(layout_binding);
        }

        {
                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = 1;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                // layout_binding.pImmutableSamplers = nullptr;

                bindings.push_back(layout_binding);
        }

        {
                VkDescriptorSetLayoutBinding layout_binding = {};
                layout_binding.binding = 2;
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                // layout_binding.pImmutableSamplers = nullptr;

                bindings.push_back(layout_binding);
        }

        return bindings;
}

std::vector<VkDeviceSize> descriptor_set_layout_bindings_sizes()
{
        std::vector<VkDeviceSize> sizes;
        sizes.push_back(sizeof(VertexShaderUniformBufferObject));
        sizes.push_back(sizeof(FragmentShaderUniformBufferObject0));
        sizes.push_back(sizeof(FragmentShaderUniformBufferObject1));
        return sizes;
}

void set_vertex_uniform(const vulkan::VulkanInstance& instance, VertexShaderUniformBufferObject ubo)
{
        instance.copy_to_buffer(0, Span(&ubo, sizeof(ubo)));
}

void set_fragment_uniform_0(const vulkan::VulkanInstance& instance, FragmentShaderUniformBufferObject0 ubo0)
{
        instance.copy_to_buffer(1, Span(&ubo0, sizeof(ubo0)));
}

void set_fragment_uniform_1(const vulkan::VulkanInstance& instance, FragmentShaderUniformBufferObject1 ubo1)
{
        instance.copy_to_buffer(2, Span(&ubo1, sizeof(ubo1)));
}

//

// clang-format off
constexpr std::array<Vertex, 4> vertices =
{
        Vertex{vec2f( 0.5, -0.5), vec3f(1, 0, 0)},
        Vertex{vec2f( 0.5, 0.5), vec3f(0, 1, 0)},
        Vertex{vec2f(-0.5, 0.5), vec3f(0, 0, 1)},
        Vertex{vec2f(-0.5, -0.5), vec3f(1, 1, 1)}
};
constexpr std::array<uint16_t, 6> vertex_indices =
{
        0, 1, 2, 2, 3, 0
};
// clang-format on

void update_uniforms(const vulkan::VulkanInstance& instance)
{
        const double radians = time_in_seconds() * 2 * PI<double>;

        FragmentShaderUniformBufferObject0 ubo0;
        ubo0.value_r = 0.5 * (1 + std::sin(radians));
        ubo0.value_g = 0.5 * (1 + std::sin(radians * 2));
        set_fragment_uniform_0(instance, ubo0);

        FragmentShaderUniformBufferObject1 ubo1;
        ubo1.value_b = 0.5 * (1 + std::sin(radians * 4));
        set_fragment_uniform_1(instance, ubo1);
}

class VulkanRendererImplementation final : public VulkanRenderer
{
        std::unique_ptr<vulkan::VulkanInstance> m_instance;

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
                m_instance->set_clear_color(color);
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
                VertexShaderUniformBufferObject ubo;
                ubo.mvp_matrix = transpose(to_matrix<float>(main_matrix));
                set_vertex_uniform(*m_instance, ubo);
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
                update_uniforms(*m_instance);

                m_instance->draw_frame();

                return true;
        }

public:
        VulkanRendererImplementation(const std::vector<std::string>& window_instance_extensions,
                                     const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        {
                LOG(vulkan_overview_for_log(window_instance_extensions));

                std::vector<std::string> instance_extensions(std::cbegin(INSTANCE_EXTENSIONS), std::cend(INSTANCE_EXTENSIONS));
                std::vector<std::string> device_extensions(std::cbegin(DEVICE_EXTENSIONS), std::cend(DEVICE_EXTENSIONS));
                std::vector<std::string> validation_layers(std::cbegin(VALIDATION_LAYERS), std::cend(VALIDATION_LAYERS));

                m_instance = std::make_unique<vulkan::VulkanInstance>(
                        API_VERSION_MAJOR, API_VERSION_MINOR, instance_extensions + window_instance_extensions, device_extensions,
                        validation_layers, create_surface, vertex_shader, fragment_shader, Vertex::binding_descriptions(),
                        Vertex::attribute_descriptions(), vertex_indices.size(), vertices.size() * sizeof(vertices[0]),
                        vertices.data(), vertex_indices.size() * sizeof(vertex_indices[0]), vertex_indices.data(),
                        descriptor_set_layout_bindings(), descriptor_set_layout_bindings_sizes());

                LOG(vulkan_overview_physical_devices_for_log(m_instance->instance()));
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
