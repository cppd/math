/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/image/image.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

namespace ns::gpu::renderer::test
{
class RayTracingImage final
{
        const vulkan::CommandPool* compute_command_pool_;
        const vulkan::Queue* compute_queue_;
        unsigned width_;
        unsigned height_;
        vulkan::ImageWithMemory image_;

public:
        RayTracingImage(
                unsigned width,
                unsigned height,
                const vulkan::Device& device,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue);

        [[nodiscard]] unsigned width() const
        {
                return width_;
        }

        [[nodiscard]] unsigned height() const
        {
                return height_;
        }

        [[nodiscard]] const vulkan::ImageView& image_view() const
        {
                return image_.image_view();
        }

        [[nodiscard]] image::Image<2> image() const;
};
}
