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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class OpticalFlowSobelMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int I_BINDING = 0;
        static constexpr int DX_BINDING = 1;
        static constexpr int DY_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        OpticalFlowSobelMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        OpticalFlowSobelMemory(const OpticalFlowSobelMemory&) = delete;
        OpticalFlowSobelMemory& operator=(const OpticalFlowSobelMemory&) = delete;
        OpticalFlowSobelMemory& operator=(OpticalFlowSobelMemory&&) = delete;

        OpticalFlowSobelMemory(OpticalFlowSobelMemory&&) = default;
        ~OpticalFlowSobelMemory() = default;

        //

        const VkDescriptorSet& descriptor_set(int index) const;

        //

        void set_i(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1);
        void set_dx(const vulkan::ImageWithMemory& image_dx);
        void set_dy(const vulkan::ImageWithMemory& image_dy);
};

class OpticalFlowSobelConstant final : public vulkan::SpecializationConstant
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
        OpticalFlowSobelConstant();

        void set(uint32_t local_size_x, uint32_t local_size_y);
};

class OpticalFlowSobelProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        OpticalFlowSobelConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit OpticalFlowSobelProgram(const vulkan::Device& device);

        OpticalFlowSobelProgram(const OpticalFlowSobelProgram&) = delete;
        OpticalFlowSobelProgram& operator=(const OpticalFlowSobelProgram&) = delete;
        OpticalFlowSobelProgram& operator=(OpticalFlowSobelProgram&&) = delete;

        OpticalFlowSobelProgram(OpticalFlowSobelProgram&&) = default;
        ~OpticalFlowSobelProgram() = default;

        void create_pipeline(uint32_t local_size_x, uint32_t local_size_y);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
