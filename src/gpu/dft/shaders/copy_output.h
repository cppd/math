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

#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class CopyOutputMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        CopyOutputMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        const VkDescriptorSet& descriptor_set() const;

        void set(const vulkan::Buffer& input, const vulkan::ImageView& output) const;
};

class CopyOutputConstant final : public vulkan::SpecializationConstant
{
        struct Data final
        {
                std::uint32_t local_size_x;
                std::uint32_t local_size_y;
                float to_mul;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        CopyOutputConstant();

        void set(std::uint32_t local_size_x, std::uint32_t local_size_y, float to_mul);
};

class CopyOutputProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        CopyOutputConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit CopyOutputProgram(VkDevice device);

        CopyOutputProgram(const CopyOutputProgram&) = delete;
        CopyOutputProgram& operator=(const CopyOutputProgram&) = delete;
        CopyOutputProgram& operator=(CopyOutputProgram&&) = delete;

        CopyOutputProgram(CopyOutputProgram&&) = default;
        ~CopyOutputProgram() = default;

        void create_pipeline(std::uint32_t local_size_x, std::uint32_t local_size_y, float to_mul);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
