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

#include "window.h"

#include "com/error.h"

VulkanWindow::VulkanWindow(int width, int height, const std::string& title)
{
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
        {
                error("Failed to create GLFW window");
        }

        m_window = window;
}

VulkanWindow::~VulkanWindow()
{
        if (m_window)
        {
                glfwDestroyWindow(m_window);
        }
}

VulkanWindow::operator GLFWwindow*() const
{
        return m_window;
}

VkSurfaceKHR VulkanWindow::create_surface(VkInstance instance)
{
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
        if (result != VK_SUCCESS || surface == VK_NULL_HANDLE)
        {
                error("Failed to create Vulkan GLFW window surface");
        }

        return surface;
}

std::vector<const char*> VulkanWindow::instance_extensions()
{
        uint32_t count;
        const char** extensions;

        extensions = glfwGetRequiredInstanceExtensions(&count);
        if (!extensions)
        {
                error("Failed to get GLFW required Vulkan instance extensions");
        }
        if (count < 1)
        {
                error("No GLFW required Vulkan instance extensions");
        }

        return std::vector<const char*>(extensions, extensions + count);
}

#endif
