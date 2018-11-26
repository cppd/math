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

#include "com/color/color.h"
#include "com/matrix.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/swapchain.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanText
{
        virtual ~VulkanText() = default;

        virtual void set_color(const Color& color) const = 0;
        virtual void set_matrix(const mat4& matrix) const = 0;

        virtual void create_buffers(const vulkan::Swapchain* swapchain, const mat4& matrix) = 0;
        virtual void delete_buffers() = 0;

        virtual void draw(VkFence queue_fence, VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore finished_semaphore,
                          unsigned image_index, int step_y, int x, int y, const std::vector<std::string>& text) = 0;

        virtual void draw(VkFence queue_fence, VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore finished_semaphore,
                          unsigned image_index, int step_y, int x, int y, const std::string& text) = 0;
};

std::unique_ptr<VulkanText> create_vulkan_text(const vulkan::VulkanInstance& instance, int size, const Color& color);
