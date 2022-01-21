/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/numerical/region.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class CopyInputMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 1;
        static constexpr int DST_BINDING = 0;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        CopyInputMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        CopyInputMemory(const CopyInputMemory&) = delete;
        CopyInputMemory& operator=(const CopyInputMemory&) = delete;
        CopyInputMemory& operator=(CopyInputMemory&&) = delete;

        CopyInputMemory(CopyInputMemory&&) = default;
        ~CopyInputMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set(VkSampler sampler, const vulkan::ImageView& input, const vulkan::Buffer& output) const;
};

class CopyInputConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::int32_t local_size_x;
                std::int32_t local_size_y;
                std::int32_t x;
                std::int32_t y;
                std::int32_t width;
                std::int32_t height;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        CopyInputConstant();

        void set(std::int32_t local_size_x, std::int32_t local_size_y, const Region<2, int>& rectangle);
};

class CopyInputProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        CopyInputConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit CopyInputProgram(VkDevice device);

        CopyInputProgram(const CopyInputProgram&) = delete;
        CopyInputProgram& operator=(const CopyInputProgram&) = delete;
        CopyInputProgram& operator=(CopyInputProgram&&) = delete;

        CopyInputProgram(CopyInputProgram&&) = default;
        ~CopyInputProgram() = default;

        void create_pipeline(std::int32_t local_size_x, std::int32_t local_size_y, const Region<2, int>& rectangle);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
