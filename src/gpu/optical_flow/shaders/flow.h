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

#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::optical_flow
{
class FlowDataBuffer final
{
        struct BufferData final
        {
                std::int32_t point_count_x;
                std::int32_t point_count_y;
                std::uint32_t use_all_points;
                std::uint32_t use_guess;
                std::int32_t guess_kx;
                std::int32_t guess_ky;
                std::int32_t guess_width;
        };

        vulkan::BufferWithMemory buffer_;

public:
        FlowDataBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        struct Data final
        {
                int point_count_x;
                int point_count_y;
                bool use_all_points;
                bool use_guess;
                int guess_kx;
                int guess_ky;
                int guess_width;
        };

        void set(const Data& data) const;
};

class FlowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int TOP_POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int POINTS_FLOW_GUESS_BINDING = 2;
        static constexpr int DATA_BINDING = 3;
        static constexpr int DX_BINDING = 4;
        static constexpr int DY_BINDING = 5;
        static constexpr int I_BINDING = 6;
        static constexpr int J_BINDING = 7;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        FlowMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, const vulkan::Buffer& data_buffer);

        [[nodiscard]] const VkDescriptorSet& descriptor_set(int index) const;

        void set_dx(const vulkan::ImageView& image) const;
        void set_dy(const vulkan::ImageView& image) const;
        void set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const;
        void set_j(VkSampler sampler, const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const;

        void set_top_points(const vulkan::Buffer& buffer) const;
        void set_flow(const vulkan::Buffer& buffer) const;
        void set_flow_guess(const vulkan::Buffer& buffer) const;
};

class FlowProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit FlowProgram(VkDevice device);

        FlowProgram(const FlowProgram&) = delete;
        FlowProgram& operator=(const FlowProgram&) = delete;
        FlowProgram& operator=(FlowProgram&&) = delete;

        FlowProgram(FlowProgram&&) = default;
        ~FlowProgram() = default;

        void create_pipeline(
                std::uint32_t local_size_x,
                std::uint32_t local_size_y,
                std::int32_t radius,
                std::int32_t max_iteration_count,
                float stop_move_square,
                float min_determinant);
        void delete_pipeline();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline() const;
};
}
