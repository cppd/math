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

#include "compute_downsample.h"

#include "shader_source.h"

#include <src/graphics/vulkan/create.h>
#include <src/graphics/vulkan/pipeline.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> OpticalFlowDownsampleMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BIG_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SMALL_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

OpticalFlowDownsampleMemory::OpticalFlowDownsampleMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned OpticalFlowDownsampleMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& OpticalFlowDownsampleMemory::descriptor_set(int index) const
{
        ASSERT(index == 0 || index == 1);
        return m_descriptors.descriptor_set(index);
}

void OpticalFlowDownsampleMemory::set_big(
        const vulkan::ImageWithMemory& image_0,
        const vulkan::ImageWithMemory& image_1) const
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, BIG_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, BIG_BINDING, image_info);
}

void OpticalFlowDownsampleMemory::set_small(
        const vulkan::ImageWithMemory& image_0,
        const vulkan::ImageWithMemory& image_1) const
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, SMALL_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, SMALL_BINDING, image_info);
}

//

OpticalFlowDownsampleConstant::OpticalFlowDownsampleConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, local_size_x);
                entry.size = sizeof(Data::local_size_x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, local_size_y);
                entry.size = sizeof(Data::local_size_y);
                m_entries.push_back(entry);
        }
}

void OpticalFlowDownsampleConstant::set(uint32_t local_size_x, uint32_t local_size_y)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;
}

const std::vector<VkSpecializationMapEntry>& OpticalFlowDownsampleConstant::entries() const
{
        return m_entries;
}

const void* OpticalFlowDownsampleConstant::data() const
{
        return &m_data;
}

size_t OpticalFlowDownsampleConstant::size() const
{
        return sizeof(m_data);
}

//

OpticalFlowDownsampleProgram::OpticalFlowDownsampleProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                  device,
                  OpticalFlowDownsampleMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {OpticalFlowDownsampleMemory::set_number()},
                  {m_descriptor_set_layout})),
          m_shader(device, optical_flow_downsample_comp(), "main")
{
}

VkDescriptorSetLayout OpticalFlowDownsampleProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout OpticalFlowDownsampleProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline OpticalFlowDownsampleProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void OpticalFlowDownsampleProgram::create_pipeline(uint32_t local_size_x, uint32_t local_size_y)
{
        m_constant.set(local_size_x, local_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void OpticalFlowDownsampleProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
