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

#include "compute_sobel.h"

#include "shader_source.h"

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> OpticalFlowSobelMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = I_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DX_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DY_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

OpticalFlowSobelMemory::OpticalFlowSobelMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned OpticalFlowSobelMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& OpticalFlowSobelMemory::descriptor_set(int index) const
{
        ASSERT(index == 0 || index == 1);
        return m_descriptors.descriptor_set(index);
}

void OpticalFlowSobelMemory::set_i(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1)
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, I_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, I_BINDING, image_info);
}

void OpticalFlowSobelMemory::set_dx(const vulkan::ImageWithMemory& image_dx)
{
        ASSERT(image_dx.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_dx.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image_dx.image_view();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, DX_BINDING, image_info);
        }
}

void OpticalFlowSobelMemory::set_dy(const vulkan::ImageWithMemory& image_dy)
{
        ASSERT(image_dy.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_dy.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image_dy.image_view();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, DY_BINDING, image_info);
        }
}

//

OpticalFlowSobelConstant::OpticalFlowSobelConstant()
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

void OpticalFlowSobelConstant::set(uint32_t local_size_x, uint32_t local_size_y)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;
}

const std::vector<VkSpecializationMapEntry>& OpticalFlowSobelConstant::entries() const
{
        return m_entries;
}

const void* OpticalFlowSobelConstant::data() const
{
        return &m_data;
}

size_t OpticalFlowSobelConstant::size() const
{
        return sizeof(m_data);
}

//

OpticalFlowSobelProgram::OpticalFlowSobelProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, OpticalFlowSobelMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {OpticalFlowSobelMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, optical_flow_sobel_comp(), "main")
{
}

VkDescriptorSetLayout OpticalFlowSobelProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout OpticalFlowSobelProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline OpticalFlowSobelProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void OpticalFlowSobelProgram::create_pipeline(uint32_t local_size_x, uint32_t local_size_y)
{
        m_constant.set(local_size_x, local_size_y);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void OpticalFlowSobelProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
