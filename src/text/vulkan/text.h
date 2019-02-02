/*
Copyright (C) 2017-2019 Topological Manifold

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
#include "com/font/text_data.h"
#include "com/matrix.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/render/render_buffer.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanText
{
        virtual ~VulkanText() = default;

        virtual void set_color(const Color& color) const = 0;

        virtual void create_buffers(vulkan::RenderBuffers2D* render_buffers, const mat4& matrix) = 0;
        virtual void delete_buffers() = 0;

        virtual void draw(VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, unsigned image_index,
                          const TextData& text_data) = 0;
};

std::unique_ptr<VulkanText> create_vulkan_text(const vulkan::VulkanInstance& instance, bool sample_shading, int size,
                                               const Color& color);
