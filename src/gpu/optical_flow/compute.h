/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <memory>

namespace ns::gpu::optical_flow
{
class Compute
{
public:
        virtual ~Compute() = default;

        virtual VkSemaphore compute(const vulkan::Queue& queue, VkSemaphore wait_semaphore) = 0;

        virtual void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const Region<2, int>& rectangle,
                unsigned top_point_count_x,
                unsigned top_point_count_y,
                const vulkan::Buffer& top_points,
                const vulkan::Buffer& top_flow) = 0;

        virtual void delete_buffers() = 0;

        virtual void reset() = 0;
};

std::unique_ptr<Compute> create_compute(
        const vulkan::Device* device,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue);
}
