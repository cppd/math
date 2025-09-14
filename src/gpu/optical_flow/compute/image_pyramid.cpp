/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "image_pyramid.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/gpu/optical_flow/barriers.h>
#include <src/gpu/optical_flow/option.h>
#include <src/gpu/optical_flow/shaders/downsample.h>
#include <src/gpu/optical_flow/shaders/grayscale.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
namespace
{
std::array<int, 2> grayscale_groups(const std::array<int, 2> group_size, const std::vector<std::array<int, 2>>& sizes)
{
        return group_count(sizes[0], group_size);
}

std::vector<std::array<int, 2>> downsample_groups(
        const std::array<int, 2> group_size,
        const std::vector<std::array<int, 2>>& sizes)
{
        std::vector<std::array<int, 2>> res;

        if (sizes.size() <= 1)
        {
                return res;
        }

        res.reserve(sizes.size() - 1);
        for (std::size_t i = 1; i < sizes.size(); ++i)
        {
                res.push_back(group_count(sizes[i], group_size));
        }
        return res;
}

std::vector<DownsampleMemory> create_downsample_memory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
{
        ASSERT(images[0].size() == images[1].size());

        std::vector<DownsampleMemory> res;
        res.reserve(images[0].size());
        for (std::size_t i = 1; i < images[0].size(); ++i)
        {
                res.emplace_back(device, descriptor_set_layout);
                res.back().set_big(images[0][i - 1].image_view(), images[1][i - 1].image_view());
                res.back().set_small(images[0][i].image_view(), images[1][i].image_view());
        }
        return res;
}
}

ImagePyramid::ImagePyramid(const VkDevice device)
        : device_(device),
          grayscale_program_(device),
          grayscale_memory_(device, grayscale_program_.descriptor_set_layout()),
          downsample_program_(device)
{
}

void ImagePyramid::create_buffers(
        const VkSampler sampler,
        const vulkan::ImageWithMemory& input,
        const numerical::Region<2, int>& rectangle,
        const std::vector<std::array<int, 2>>& sizes,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
{
        grayscale_groups_ = grayscale_groups(GROUP_SIZE, sizes);
        downsample_groups_ = downsample_groups(GROUP_SIZE, sizes);

        grayscale_program_.create_pipeline(GROUP_SIZE[0], GROUP_SIZE[1], rectangle);
        grayscale_memory_.set_src(sampler, input.image_view());
        grayscale_memory_.set_dst(images[0][0].image_view(), images[1][0].image_view());

        downsample_program_.create_pipeline(GROUP_SIZE[0], GROUP_SIZE[1]);
        downsample_memory_ = create_downsample_memory(device_, downsample_program_.descriptor_set_layout(), images);
}

void ImagePyramid::delete_buffers()
{
        grayscale_program_.delete_pipeline();
        downsample_program_.delete_pipeline();

        downsample_memory_.clear();
}

void ImagePyramid::commands(
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const int index,
        const VkCommandBuffer command_buffer) const
{
        ASSERT(index == 0 || index == 1);
        ASSERT(downsample_memory_.size() == downsample_groups_.size());
        ASSERT(downsample_memory_.size() + 1 == images[index].size());

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, grayscale_program_.pipeline());
        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, grayscale_program_.pipeline_layout(),
                GrayscaleMemory::set_number(), 1, &grayscale_memory_.descriptor_set(index), 0, nullptr);
        vkCmdDispatch(command_buffer, grayscale_groups_[0], grayscale_groups_[1], 1);

        image_barrier(
                command_buffer, images[index][0].image().handle(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

        for (std::size_t i = 0; i < downsample_groups_.size(); ++i)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsample_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsample_program_.pipeline_layout(),
                        DownsampleMemory::set_number(), 1, &downsample_memory_[i].descriptor_set(index), 0, nullptr);
                vkCmdDispatch(command_buffer, downsample_groups_[i][0], downsample_groups_[i][1], 1);

                image_barrier(
                        command_buffer, images[index][i + 1].image().handle(), VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
        }
}
}
