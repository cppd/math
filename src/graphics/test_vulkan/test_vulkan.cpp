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

#include "test_vulkan.h"

#include "com/log.h"
#include "com/vec.h"
#include "graphics/vulkan/common.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/query.h"
#include "graphics/vulkan/window.h"

#include <array>
#include <cmath>
#include <thread>

constexpr double WINDOW_SIZE_COEF = 0.5;

// clang-format off
constexpr uint32_t vertex_shader[]
{
#include "test_vulkan.vert.spr"
};
constexpr uint32_t fragment_shader[]
{
#include "test_vulkan.frag.spr"
};
// clang-format on

namespace
{
std::array<int, 2> window_size()
{
        std::array<int, 2> size = VulkanWindow::screen_size();

        size[0] = std::round(size[0] * WINDOW_SIZE_COEF);
        size[1] = std::round(size[1] * WINDOW_SIZE_COEF);

        return size;
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

// clang-format off
constexpr std::array<Vertex, 3> vertices =
{
        Vertex{vec2f( 0.0,  0.9), vec3f(1, 0, 0)},
        Vertex{vec2f( 0.9, -0.9), vec3f(0, 1, 0)},
        Vertex{vec2f(-0.9, -0.9), vec3f(0, 0, 1)}
};
// clang-format on

void test_vulkan_thread()
{
        try
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

                vulkan::VulkanInstance vulkan_instance(
                        1, 0, instance_extensions + window_instance_extensions, device_extensions, validation_layers,
                        [&window](VkInstance instance) { return window.create_surface(instance); }, vertex_shader,
                        fragment_shader, vertices.size() * sizeof(Vertex), vertices.data(), vertices.size(),
                        Vertex::binding_descriptions(), Vertex::attribute_descriptions());

                LOG(vulkan::overview_physical_devices(vulkan_instance.instance()));

                while (!glfwWindowShouldClose(window))
                {
                        glfwPollEvents();

                        vulkan_instance.draw_frame();
                }

                vulkan_instance.device_wait_idle();
        }
        catch (std::exception& e)
        {
                LOG(std::string("Vulkan window test error: ") + e.what());
        }
        catch (...)
        {
                LOG("Vulkan window test unknown error");
        }
}
}

void test_vulkan_window()
{
        std::thread thread([]() noexcept { test_vulkan_thread(); });
        thread.join();
}

#endif
