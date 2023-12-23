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

#include <memory>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::pencil_sketch
{
class Compute
{
public:
        virtual ~Compute() = default;

        virtual void compute_commands(VkCommandBuffer command_buffer) const = 0;

        virtual void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle,
                const vulkan::ImageWithMemory& output) = 0;

        virtual void delete_buffers() = 0;
};

std::unique_ptr<Compute> create_compute(const vulkan::Device* device);
}
