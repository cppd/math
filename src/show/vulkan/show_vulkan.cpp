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

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)

#include "show_vulkan.h"

#include "com/log.h"
#include "com/math.h"
#include "com/time.h"
#include "com/vec.h"
#include "graphics/vulkan/common.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/query.h"
#include "graphics/vulkan/window.h"
#include "window/window_prop.h"

#include <array>
#include <atomic>
#include <cmath>
#include <thread>

constexpr double WINDOW_SIZE_COEF = 0.5;

// clang-format off
constexpr uint32_t vertex_shader[]
{
#include "draw_vulkan.vert.spr"
};
constexpr uint32_t fragment_shader[]
{
#include "draw_vulkan.frag.spr"
};
// clang-format on

namespace
{
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
                layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
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

        return bindings;
}

std::vector<VkDeviceSize> descriptor_set_layout_bindings_sizes()
{
        std::vector<VkDeviceSize> sizes;
        sizes.push_back(sizeof(FragmentShaderUniformBufferObject0));
        sizes.push_back(sizeof(FragmentShaderUniformBufferObject1));
        return sizes;
}

void set_fragment_uniform_0(const vulkan::VulkanInstance& instance, FragmentShaderUniformBufferObject0 ubo0)
{
        instance.copy_to_buffer(0, Span(&ubo0, sizeof(ubo0)));
}

void set_fragment_uniform_1(const vulkan::VulkanInstance& instance, FragmentShaderUniformBufferObject1 ubo1)
{
        instance.copy_to_buffer(1, Span(&ubo1, sizeof(ubo1)));
}

//

// clang-format off
constexpr std::array<Vertex, 4> vertices =
{
        Vertex{vec2f( 0.9,  0.9), vec3f(1, 0, 0)},
        Vertex{vec2f( 0.9, -0.9), vec3f(0, 1, 0)},
        Vertex{vec2f(-0.9, -0.9), vec3f(0, 0, 1)},
        Vertex{vec2f(-0.9,  0.9), vec3f(1, 1, 1)}
};
constexpr std::array<uint16_t, 6> indices =
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

std::array<int, 2> window_size()
{
        std::array<int, 2> size = VulkanWindow::screen_size();

        size[0] = std::round(size[0] * WINDOW_SIZE_COEF);
        size[1] = std::round(size[1] * WINDOW_SIZE_COEF);

        return size;
}

class VulkanObject final : public IShowVulkan
{
        WindowID m_win_parent;
        std::thread m_thread;
        std::atomic_bool m_stop{false};

        void loop();
        void loop_thread();

        void parent_resized() override
        {
                LOG("parent resized");
        }

        void mouse_wheel(double) override
        {
                LOG("mouse wheel");
        }

public:
        VulkanObject(WindowID win_parent) : m_win_parent(win_parent)
        {
                m_thread = std::thread(&VulkanObject::loop_thread, this);
        }

        ~VulkanObject() override
        {
                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }
};

void VulkanObject::loop()
{
        const std::vector<std::string> instance_extensions({});
        const std::vector<std::string> device_extensions({});

        const std::vector<std::string> window_instance_extensions(VulkanWindow::instance_extensions());

        const std::vector<std::string> validation_layers({"VK_LAYER_LUNARG_standard_validation"});

        if (window_instance_extensions.size() > 0)
        {
                LOG("Window instance extensions");
                for (const std::string& s : window_instance_extensions)
                {
                        LOG(std::string("  ") + s);
                }
        }

        LOG(vulkan::overview());

        VulkanWindow window(window_size(), "Vulkan Window");
        move_window_to_parent(window.get_system_handle(), m_win_parent);

        constexpr unsigned vertex_count = indices.size();

        vulkan::VulkanInstance vulkan_instance(
                1, 0, instance_extensions + window_instance_extensions, device_extensions, validation_layers,
                [&window](VkInstance instance) { return window.create_surface(instance); }, vertex_shader, fragment_shader,
                Vertex::binding_descriptions(), Vertex::attribute_descriptions(), vertex_count,
                vertices.size() * sizeof(vertices[0]), vertices.data(), indices.size() * sizeof(indices[0]), indices.data(),
                descriptor_set_layout_bindings(), descriptor_set_layout_bindings_sizes());

        LOG(vulkan::overview_physical_devices(vulkan_instance.instance()));

        while (true)
        {
                if (m_stop)
                {
                        return;
                }

                // if (glfwWindowShouldClose(window))
                // {
                //        return;
                //}

                glfwPollEvents();

                update_uniforms(vulkan_instance);

                vulkan_instance.draw_frame();
        }
}

void VulkanObject::loop_thread()
{
        try
        {
                loop();
        }
        catch (std::exception& e)
        {
                error_fatal(e.what());
        }
        catch (...)
        {
                error_fatal("Unknown Error. Thread ended.");
        }
}
}

std::unique_ptr<IShowVulkan> create_show_vulkan(WindowID win_parent)
{
        return std::make_unique<VulkanObject>(win_parent);
}

#endif
