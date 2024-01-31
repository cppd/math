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

#include "sobel.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/gpu/optical_flow/barriers.h>
#include <src/gpu/optical_flow/option.h>
#include <src/gpu/optical_flow/shaders/sobel.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
namespace
{
std::vector<numerical::Vector2i> sobel_groups(
        const numerical::Vector2i group_size,
        const std::vector<numerical::Vector2i>& sizes)
{
        std::vector<numerical::Vector2i> res;
        res.reserve(sizes.size());
        for (const numerical::Vector2i& size : sizes)
        {
                res.push_back(group_count(size, group_size));
        }
        return res;
}

std::vector<SobelMemory> create_sobel_memory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy)
{
        ASSERT(images[0].size() == images[1].size());
        ASSERT(images[0].size() == dx.size());
        ASSERT(images[0].size() == dy.size());

        std::vector<SobelMemory> res;
        res.reserve(images[0].size());
        for (std::size_t i = 0; i < images[0].size(); ++i)
        {
                res.emplace_back(device, descriptor_set_layout);
                res.back().set_i(images[0][i].image_view(), images[1][i].image_view());
                res.back().set_dx(dx[i].image_view());
                res.back().set_dy(dy[i].image_view());
        }
        return res;
}
}

Sobel::Sobel(const VkDevice device)
        : device_(device),
          sobel_program_(device_)
{
}

void Sobel::create_buffers(
        const std::vector<numerical::Vector2i>& sizes,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
{
        sobel_groups_ = sobel_groups(GROUP_SIZE, sizes);
        sobel_program_.create_pipeline(GROUP_SIZE[0], GROUP_SIZE[1]);
        sobel_memory_ = create_sobel_memory(device_, sobel_program_.descriptor_set_layout(), images, dx, dy);
}

void Sobel::delete_buffers()
{
        sobel_program_.delete_pipeline();
        sobel_memory_.clear();
}

void Sobel::commands(
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy,
        const int index,
        const VkCommandBuffer command_buffer) const
{
        ASSERT(index == 0 || index == 1);
        ASSERT(sobel_memory_.size() == sobel_groups_.size());
        ASSERT(sobel_groups_.size() == dx.size());
        ASSERT(sobel_groups_.size() == dy.size());

        for (std::size_t i = 0; i < sobel_groups_.size(); ++i)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, sobel_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, sobel_program_.pipeline_layout(),
                        SobelMemory::set_number(), 1, &sobel_memory_[i].descriptor_set(index), 0, nullptr);
                vkCmdDispatch(command_buffer, sobel_groups_[i][0], sobel_groups_[i][1], 1);
        }

        std::vector<VkImage> images;
        images.reserve(dx.size() + dy.size());
        for (std::size_t i = 0; i < sobel_groups_.size(); ++i)
        {
                images.push_back(dx[i].image().handle());
                images.push_back(dy[i].image().handle());
        }

        image_barrier(
                command_buffer, images, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT);
}
}
