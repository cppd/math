/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <src/vulkan/instance.h>

#include <complex>
#include <memory>
#include <vector>

namespace gpu_vulkan
{
struct DftComputeImage
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~DftComputeImage() = default;

        virtual void compute_commands(VkCommandBuffer command_buffer) const = 0;
        virtual void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& output,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height,
                uint32_t family_index) = 0;
        virtual void delete_buffers() = 0;
};

struct DftComputeVector
{
        virtual ~DftComputeVector() = default;

        virtual void create_buffers(unsigned width, unsigned height) = 0;
        virtual void exec(bool inverse, std::vector<std::complex<float>>* src) = 0;
};

std::unique_ptr<DftComputeImage> create_dft_compute_image(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue);

std::unique_ptr<DftComputeVector> create_dft_compute_vector();
}
