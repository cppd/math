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

#include "flow.h"

#include "create.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/gpu/optical_flow/barriers.h>
#include <src/gpu/optical_flow/option.h>
#include <src/gpu/optical_flow/shaders/flow.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
namespace
{
std::vector<const vulkan::Buffer*> to_buffer_pointers(const std::vector<vulkan::BufferWithMemory>& buffers)
{
        std::vector<const vulkan::Buffer*> res;
        res.reserve(buffers.size());
        for (const vulkan::BufferWithMemory& buffer : buffers)
        {
                res.push_back(&buffer.buffer());
        }
        return res;
}

std::vector<std::array<int, 2>> flow_groups(
        const std::array<int, 2> group_size,
        const std::vector<std::array<int, 2>>& sizes,
        const std::array<int, 2> top_point_count)
{
        std::vector<std::array<int, 2>> res;
        res.reserve(sizes.size());

        res.push_back(group_count(top_point_count, group_size));

        for (std::size_t i = 1; i < sizes.size(); ++i)
        {
                res.push_back(group_count(sizes[i], group_size));
        }

        return res;
}
}

Flow::Flow(const vulkan::Device* const device)
        : device_(device),
          flow_program_(device_->handle())
{
}

void Flow::create_buffers(
        const VkSampler sampler,
        const std::uint32_t family_index,
        const std::vector<std::array<int, 2>>& sizes,
        const int top_point_count_x,
        const int top_point_count_y,
        const vulkan::Buffer& top_points,
        const vulkan::Buffer& top_flow,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy)
{
        flow_buffers_ = create_flow_buffers(*device_, sizes, family_index);

        flow_groups_ = flow_groups(GROUP_SIZE, sizes, {top_point_count_x, top_point_count_y});

        flow_program_.create_pipeline(
                GROUP_SIZE[0], GROUP_SIZE[1], RADIUS, MAX_ITERATION_COUNT, STOP_MOVE_SQUARE, MIN_DETERMINANT);

        std::tie(flow_buffer_, flow_memory_) = create_flow_memory(
                *device_, flow_program_.descriptor_set_layout(), family_index, sampler, sizes,
                to_buffer_pointers(flow_buffers_), top_point_count_x, top_point_count_y, top_points, top_flow, images,
                dx, dy);
}

void Flow::delete_buffers()
{
        flow_program_.delete_pipeline();
        flow_buffers_.clear();
        flow_memory_.clear();
}

void Flow::commands(const int index, const VkCommandBuffer command_buffer, const VkBuffer top_flow) const
{
        ASSERT(index == 0 || index == 1);
        ASSERT(flow_memory_.size() == flow_groups_.size());
        ASSERT(flow_buffers_.size() + 1 == flow_groups_.size());

        for (int i = static_cast<int>(flow_groups_.size()) - 1; i >= 0; --i)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, flow_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, flow_program_.pipeline_layout(),
                        FlowMemory::set_number(), 1, &flow_memory_[i].descriptor_set(index), 0, nullptr);
                vkCmdDispatch(command_buffer, flow_groups_[i][0], flow_groups_[i][1], 1);

                buffer_barrier(
                        command_buffer, (i != 0) ? flow_buffers_[i - 1].buffer().handle() : top_flow,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
}
}
