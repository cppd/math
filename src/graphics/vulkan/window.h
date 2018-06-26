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

#pragma once

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)

#include <string>
#include <vector>

// clang-format off
// Перед включением GLFW/glfw3.h надо включить vulkan/vulkan.h
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
// clang-format on

class VulkanWindow
{
        GLFWwindow* m_window = nullptr;

public:
        VulkanWindow(int width, int height, const std::string& title);
        ~VulkanWindow();

        operator GLFWwindow*() const;

        VkSurfaceKHR create_surface(VkInstance instance);

        static std::vector<const char*> instance_extensions();

        VulkanWindow(const VulkanWindow&) = delete;
        VulkanWindow& operator=(const VulkanWindow&) = delete;
        VulkanWindow(VulkanWindow&&) = delete;
        VulkanWindow& operator=(VulkanWindow&&) = delete;
};

#endif
