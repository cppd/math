/*
Copyright (C) 2017-2021 Topological Manifold

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

namespace ns::gpu::optical_flow
{
class SobelMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int I_BINDING = 0;
        static constexpr int DX_BINDING = 1;
        static constexpr int DY_BINDING = 2;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        SobelMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        SobelMemory(const SobelMemory&) = delete;
        SobelMemory& operator=(const SobelMemory&) = delete;
        SobelMemory& operator=(SobelMemory&&) = delete;

        SobelMemory(SobelMemory&&) = default;
        ~SobelMemory() = default;

        //

        const VkDescriptorSet& descriptor_set(int index) const;

        //

        void set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1);
        void set_dx(const vulkan::ImageView& image_dx);
        void set_dy(const vulkan::ImageView& image_dy);
};

class SobelConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::uint32_t local_size_x;
                std::uint32_t local_size_y;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        SobelConstant();

        void set(std::uint32_t local_size_x, std::uint32_t local_size_y);
};

class SobelProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        SobelConstant constant_;
        vulkan::ComputeShader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit SobelProgram(VkDevice device);

        SobelProgram(const SobelProgram&) = delete;
        SobelProgram& operator=(const SobelProgram&) = delete;
        SobelProgram& operator=(SobelProgram&&) = delete;

        SobelProgram(SobelProgram&&) = default;
        ~SobelProgram() = default;

        void create_pipeline(std::uint32_t local_size_x, std::uint32_t local_size_y);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
