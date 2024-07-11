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

#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/physical_device.h>

#include <vulkan/vulkan_core.h>

#include <complex>
#include <cstdint>
#include <memory>
#include <vector>

namespace ns::gpu::dft
{
class ComputeImage
{
public:
        virtual ~ComputeImage() = default;

        virtual void compute_commands(VkCommandBuffer command_buffer) const = 0;

        virtual void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& output,
                const numerical::Region<2, int>& rectangle,
                std::uint32_t family_index) = 0;

        virtual void delete_buffers() = 0;
};

class ComputeVector
{
public:
        virtual ~ComputeVector() = default;

        virtual void create_buffers(unsigned width, unsigned height) = 0;

        virtual void exec(bool inverse, std::vector<std::complex<float>>* src) = 0;
};

std::unique_ptr<ComputeImage> create_compute_image(
        const vulkan::Device* device,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue);

std::unique_ptr<ComputeVector> create_compute_vector(vulkan::physical_device::PhysicalDeviceSearchType search_type);
}
