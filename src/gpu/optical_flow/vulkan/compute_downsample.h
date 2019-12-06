/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class OpticalFlowDownsampleMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BIG_BINDING = 0;
        static constexpr int SMALL_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        OpticalFlowDownsampleMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        OpticalFlowDownsampleMemory(const OpticalFlowDownsampleMemory&) = delete;
        OpticalFlowDownsampleMemory& operator=(const OpticalFlowDownsampleMemory&) = delete;
        OpticalFlowDownsampleMemory& operator=(OpticalFlowDownsampleMemory&&) = delete;

        OpticalFlowDownsampleMemory(OpticalFlowDownsampleMemory&&) = default;
        ~OpticalFlowDownsampleMemory() = default;

        //

        const VkDescriptorSet& descriptor_set(int index) const;

        //

        void set_big(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1) const;
        void set_small(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1) const;
};

class OpticalFlowDownsampleConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        OpticalFlowDownsampleConstant();

        void set(uint32_t local_size_x, uint32_t local_size_y);
};

class OpticalFlowDownsampleProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        OpticalFlowDownsampleConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        OpticalFlowDownsampleProgram(const vulkan::Device& device);

        OpticalFlowDownsampleProgram(const OpticalFlowDownsampleProgram&) = delete;
        OpticalFlowDownsampleProgram& operator=(const OpticalFlowDownsampleProgram&) = delete;
        OpticalFlowDownsampleProgram& operator=(OpticalFlowDownsampleProgram&&) = delete;

        OpticalFlowDownsampleProgram(OpticalFlowDownsampleProgram&&) = default;
        ~OpticalFlowDownsampleProgram() = default;

        void create_pipeline(uint32_t local_size_x, uint32_t local_size_y);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
