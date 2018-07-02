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

#include "com/error.h"
#include "com/log.h"
#include "graphics/vulkan/common.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/query.h"
#include "graphics/vulkan/window.h"

#include <memory>
#include <thread>

constexpr int WINDOW_WIDTH = 1024;
constexpr int WINDOW_HEIGHT = 576;

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

                VulkanWindow window(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Window");

                vulkan::VulkanInstance vulkan_instance(1, 0, instance_extensions + window_instance_extensions, device_extensions,
                                                       validation_layers,
                                                       [&window](VkInstance instance) { return window.create_surface(instance); },
                                                       vertex_shader, fragment_shader);

                LOG(vulkan::overview_physical_devices(vulkan_instance.instance()));

                while (!glfwWindowShouldClose(window))
                {
                        glfwPollEvents();
                }
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
