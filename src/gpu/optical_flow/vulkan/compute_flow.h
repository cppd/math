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
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu::optical_flow
{
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

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct BufferData
        {
                int32_t point_count_x;
                int32_t point_count_y;
                uint32_t use_all_points;
                uint32_t use_guess;
                int32_t guess_kx;
                int32_t guess_ky;
                int32_t guess_width;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        FlowMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::unordered_set<uint32_t>& family_indices);

        FlowMemory(const FlowMemory&) = delete;
        FlowMemory& operator=(const FlowMemory&) = delete;
        FlowMemory& operator=(FlowMemory&&) = delete;

        FlowMemory(FlowMemory&&) = default;
        ~FlowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set(int index) const;

        //

        struct Data
        {
                int point_count_x;
                int point_count_y;
                bool use_all_points;
                bool use_guess;
                int guess_kx;
                int guess_ky;
                int guess_width;
        };

        void set_data(const Data& data) const;

        void set_dx(const vulkan::ImageWithMemory& image) const;
        void set_dy(const vulkan::ImageWithMemory& image) const;
        void set_i(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1) const;
        void set_j(VkSampler sampler, const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1)
                const;

        void set_top_points(const vulkan::BufferWithMemory& buffer) const;
        void set_flow(const vulkan::BufferWithMemory& buffer) const;
        void set_flow_guess(const vulkan::BufferWithMemory& buffer) const;
};

class FlowConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
                int32_t radius;
                int32_t iteration_count;
                float stop_move_square;
                float min_determinant;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        FlowConstant();

        void set(
                uint32_t local_size_x,
                uint32_t local_size_y,
                int32_t radius,
                int32_t iteration_count,
                float stop_move_square,
                float min_determinant);
};

class FlowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        FlowConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit FlowProgram(const vulkan::Device& device);

        FlowProgram(const FlowProgram&) = delete;
        FlowProgram& operator=(const FlowProgram&) = delete;
        FlowProgram& operator=(FlowProgram&&) = delete;

        FlowProgram(FlowProgram&&) = default;
        ~FlowProgram() = default;

        void create_pipeline(
                uint32_t local_size_x,
                uint32_t local_size_y,
                int32_t radius,
                int32_t iteration_count,
                float stop_move_square,
                float min_determinant);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
