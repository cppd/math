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

namespace gpu_vulkan
{
class DftCopyOutputMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        DftCopyOutputMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        DftCopyOutputMemory(const DftCopyOutputMemory&) = delete;
        DftCopyOutputMemory& operator=(const DftCopyOutputMemory&) = delete;
        DftCopyOutputMemory& operator=(DftCopyOutputMemory&&) = delete;

        DftCopyOutputMemory(DftCopyOutputMemory&&) = default;
        ~DftCopyOutputMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set(const vulkan::BufferWithMemory& input, const vulkan::ImageWithMemory& output) const;
};

class DftCopyOutputConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
                float to_mul;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        DftCopyOutputConstant();

        void set(uint32_t local_size_x, uint32_t local_size_y, float to_mul);
};

class DftCopyOutputProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        DftCopyOutputConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit DftCopyOutputProgram(const vulkan::Device& device);

        DftCopyOutputProgram(const DftCopyOutputProgram&) = delete;
        DftCopyOutputProgram& operator=(const DftCopyOutputProgram&) = delete;
        DftCopyOutputProgram& operator=(DftCopyOutputProgram&&) = delete;

        DftCopyOutputProgram(DftCopyOutputProgram&&) = default;
        ~DftCopyOutputProgram() = default;

        void create_pipeline(uint32_t local_size_x, uint32_t local_size_y, float to_mul);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
