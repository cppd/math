/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/vulkan/buffers.h>

namespace ns::gpu::renderer
{
class RayTracingImage
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
                const vulkan::Device* device,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue);

        unsigned width() const
        {
                return width_;
        }

        unsigned height() const
        {
                return height_;
        }

        const vulkan::ImageView& image_view() const
        {
                return image_.image_view();
        }

        void save_to_file(const std::string_view& name) const;
};
}
