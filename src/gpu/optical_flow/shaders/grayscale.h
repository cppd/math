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

#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::optical_flow
{
class GrayscaleMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        GrayscaleMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        GrayscaleMemory(const GrayscaleMemory&) = delete;
        GrayscaleMemory& operator=(const GrayscaleMemory&) = delete;
        GrayscaleMemory& operator=(GrayscaleMemory&&) = delete;

        GrayscaleMemory(GrayscaleMemory&&) = default;
        ~GrayscaleMemory() = default;

        //

        const VkDescriptorSet& descriptor_set(int index) const;

        //

        void set_src(VkSampler sampler, const vulkan::ImageWithMemory& image);
        void set_dst(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1);
};

class GrayscaleConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
                int32_t x;
                int32_t y;
                int32_t width;
                int32_t height;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        GrayscaleConstant();

        void set(uint32_t local_size_x, uint32_t local_size_y, const Region<2, int>& rectangle);
};

class GrayscaleProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        GrayscaleConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit GrayscaleProgram(const vulkan::Device& device);

        GrayscaleProgram(const GrayscaleProgram&) = delete;
        GrayscaleProgram& operator=(const GrayscaleProgram&) = delete;
        GrayscaleProgram& operator=(GrayscaleProgram&&) = delete;

        GrayscaleProgram(GrayscaleProgram&&) = default;
        ~GrayscaleProgram() = default;

        void create_pipeline(uint32_t local_size_x, uint32_t local_size_y, const Region<2, int>& rectangle);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
