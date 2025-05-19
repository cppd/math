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

#pragma once

#include <src/gpu/optical_flow/shaders/flow.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
class Flow final
{
        const vulkan::Device* const device_;

        FlowProgram flow_program_;
        std::vector<FlowDataBuffer> flow_buffer_;
        std::vector<FlowMemory> flow_memory_;
        std::vector<std::array<int, 2>> flow_groups_;

        std::vector<vulkan::BufferWithMemory> flow_buffers_;

public:
        explicit Flow(const vulkan::Device* device);

        void create_buffers(
                VkSampler sampler,
                std::uint32_t family_index,
                const std::vector<std::array<int, 2>>& sizes,
                int top_point_count_x,
                int top_point_count_y,
                const vulkan::Buffer& top_points,
                const vulkan::Buffer& top_flow,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy);

        void delete_buffers();

        void commands(int index, VkCommandBuffer command_buffer, VkBuffer top_flow) const;
};
}
