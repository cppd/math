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

#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#error This operating system is not supported
#endif
#include <GLFW/glfw3native.h>

VulkanWindow::VulkanWindow(const std::array<int, 2>& size, const std::string& title)
{
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        GLFWwindow* window = glfwCreateWindow(size[0], size[1], title.c_str(), nullptr, nullptr);
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

WindowID VulkanWindow::get_system_handle()
{
#if defined(__linux__)
        return glfwGetX11Window(m_window);
#elif defined(_WIN32)
        return glfwGetWin32Window(m_window);
#else
#error This operating system is not supported
#endif
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

std::vector<std::string> VulkanWindow::instance_extensions()
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

        return std::vector<std::string>(extensions, extensions + count);
}

std::array<int, 2> VulkanWindow::screen_size()
{
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
                error("Failed to find GLFW monitor");
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
                error("Failed to find GLFW monitor video mode");
        }

        return {mode->width, mode->height};
}

#endif
