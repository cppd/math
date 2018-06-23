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
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/window.h"

#include <memory>
#include <thread>

constexpr int WINDOW_WIDTH = 1024;
constexpr int WINDOW_HEIGHT = 576;

namespace
{
std::vector<const char*> operator+(const std::vector<const char*>& v1, const std::vector<const char*>& v2)
{
        std::vector<const char*> res;
        res.reserve(v1.size() + v2.size());
        for (const char* s : v1)
        {
                res.push_back(s);
        }
        for (const char* s : v2)
        {
                res.push_back(s);
        }
        return res;
}

void test_vulkan_thread()
{
        try
        {
                const std::vector<const char*> instance_extensions({});
                const std::vector<const char*> device_extensions({});

                const std::vector<const char*> window_instance_extensions(VulkanWindow::instance_extensions());
                const std::vector<const char*> window_device_extensions(VulkanWindow::device_extensions());

                const std::vector<const char*> validation_layers({"VK_LAYER_LUNARG_standard_validation"});

                if (window_instance_extensions.size() > 0)
                {
                        LOG("Window instance extensions");
                        for (const char* s : window_instance_extensions)
                        {
                                LOG(std::string("  ") + s);
                        }
                }

                if (window_device_extensions.size() > 0)
                {
                        LOG("Window device extensions");
                        for (const char* s : window_device_extensions)
                        {
                                LOG(std::string("  ") + s);
                        }
                }

                LOG(vulkan::overview());

                VulkanWindow window(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Window");

                vulkan::VulkanInstance vulkan_instance(
                        1, 0, instance_extensions + window_instance_extensions, device_extensions + window_device_extensions,
                        validation_layers, [&window](VkInstance instance) { return window.create_surface(instance); });

                LOG(vulkan::overview_physical_devices(vulkan_instance));

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
