/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "create.h"

#include <src/com/error.h>
#include <src/gpu/optical_flow/shaders/flow.h>
#include <src/numerical/vector.h>
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
struct FlowInfo final
{
        const vulkan::Buffer* top_points_ptr;
        const vulkan::Buffer* flow_ptr;
        const vulkan::Buffer* flow_guess_ptr;
        FlowDataBuffer::Data data;
};

FlowInfo flow_info(
        const std::size_t i,
        const vulkan::Buffer& top_points,
        const vulkan::Buffer& top_flow,
        const std::vector<const vulkan::Buffer*>& flow_buffers,
        const std::vector<std::array<int, 2>>& sizes,
        const int top_point_count_x,
        const int top_point_count_y)
{
        const auto flow_index = [&](const std::size_t index)
        {
                ASSERT(index > 0 && index < sizes.size());
                return index - 1; // buffers start at level 1
        };

        const bool top = (i == 0);
        const bool bottom = (i + 1 == sizes.size());

        FlowInfo res;

        if (!top)
        {
                res.top_points_ptr = &top_points; // not used
                res.flow_ptr = flow_buffers[flow_index(i)];
                res.data.use_all_points = true;
                res.data.point_count_x = sizes[i][0];
                res.data.point_count_y = sizes[i][1];
        }
        else
        {
                res.top_points_ptr = &top_points;
                res.flow_ptr = &top_flow;
                res.data.use_all_points = false;
                res.data.point_count_x = top_point_count_x;
                res.data.point_count_y = top_point_count_y;
        }

        if (!bottom)
        {
                const int i_prev = i + 1;
                res.data.use_guess = true;
                res.data.guess_kx = (sizes[i_prev][0] != sizes[i][0]) ? 2 : 1;
                res.data.guess_ky = (sizes[i_prev][1] != sizes[i][1]) ? 2 : 1;
                res.data.guess_width = sizes[i_prev][0];
                res.flow_guess_ptr = flow_buffers[flow_index(i_prev)];
        }
        else
        {
                res.flow_guess_ptr = flow_buffers[0]; // not used
                res.data.use_guess = false;
        }

        return res;
}
}

std::vector<vulkan::BufferWithMemory> create_flow_buffers(
        const vulkan::Device& device,
        const std::vector<std::array<int, 2>>& sizes,
        const std::uint32_t family_index)
{
        std::vector<vulkan::BufferWithMemory> res;

        if (sizes.size() <= 1)
        {
                return res;
        }

        res.reserve(sizes.size() - 1);

        const std::vector<std::uint32_t> family_indices({family_index});
        for (std::size_t i = 1; i < sizes.size(); ++i)
        {
                const std::size_t buffer_size = sizeof(numerical::Vector2f) * sizes[i][0] * sizes[i][1];
                res.emplace_back(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);
        }

        return res;
}

std::tuple<std::vector<FlowDataBuffer>, std::vector<FlowMemory>> create_flow_memory(
        const vulkan::Device& device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::uint32_t family_index,
        const VkSampler sampler,
        const std::vector<std::array<int, 2>>& sizes,
        const std::vector<const vulkan::Buffer*>& flow_buffers,
        const int top_point_count_x,
        const int top_point_count_y,
        const vulkan::Buffer& top_points,
        const vulkan::Buffer& top_flow,
        const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
        const std::vector<vulkan::ImageWithMemory>& dx,
        const std::vector<vulkan::ImageWithMemory>& dy)
{
        const std::size_t size = sizes.size();

        std::tuple<std::vector<FlowDataBuffer>, std::vector<FlowMemory>> res;

        if (size <= 1)
        {
                return res;
        }

        ASSERT(images[0].size() == size);
        ASSERT(images[1].size() == size);
        ASSERT(dx.size() == size);
        ASSERT(dy.size() == size);
        ASSERT(flow_buffers.size() + 1 == size);

        const std::vector<std::uint32_t> family_indices{family_index};

        std::get<0>(res).reserve(size);
        std::get<1>(res).reserve(size);

        for (std::size_t i = 0; i < size; ++i)
        {
                const FlowInfo info =
                        flow_info(i, top_points, top_flow, flow_buffers, sizes, top_point_count_x, top_point_count_y);

                const FlowDataBuffer& buffer = std::get<0>(res).emplace_back(device, family_indices);

                const FlowMemory& memory =
                        std::get<1>(res).emplace_back(device.handle(), descriptor_set_layout, buffer.buffer());

                buffer.set(info.data);

                memory.set_top_points(*info.top_points_ptr);
                memory.set_flow(*info.flow_ptr);
                memory.set_flow_guess(*info.flow_guess_ptr);

                memory.set_dx(dx[i].image_view());
                memory.set_dy(dy[i].image_view());
                memory.set_i(images[0][i].image_view(), images[1][i].image_view());
                memory.set_j(sampler, images[1][i].image_view(), images[0][i].image_view());
        }

        return res;
}
}
