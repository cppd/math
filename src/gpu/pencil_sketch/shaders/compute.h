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

namespace ns::gpu::pencil_sketch
{
class ComputeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int INPUT_BINDING = 0;
        static constexpr int OUTPUT_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        //

        ComputeMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        ComputeMemory(const ComputeMemory&) = delete;
        ComputeMemory& operator=(const ComputeMemory&) = delete;
        ComputeMemory& operator=(ComputeMemory&&) = delete;

        ComputeMemory(ComputeMemory&&) = default;
        ~ComputeMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
        void set_output_image(const vulkan::ImageWithMemory& image) const;
        void set_object_image(const vulkan::ImageWithMemory& image) const;
};

class ComputeConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t local_size;
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
        ComputeConstant();

        void set(int32_t local_size, const Region<2, int>& rectangle);
};

class ComputeProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        ComputeConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::Pipeline m_pipeline;

public:
        explicit ComputeProgram(const vulkan::Device& device);

        ComputeProgram(const ComputeProgram&) = delete;
        ComputeProgram& operator=(const ComputeProgram&) = delete;
        ComputeProgram& operator=(ComputeProgram&&) = delete;

        ComputeProgram(ComputeProgram&&) = default;
        ~ComputeProgram() = default;

        void create_pipeline(unsigned group_size, const Region<2, int>& rectangle);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
