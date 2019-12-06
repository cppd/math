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

#include "compute_grayscale.h"

#include "shader_source.h"

#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> OpticalFlowGrayscaleMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SRC_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DST_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

OpticalFlowGrayscaleMemory::OpticalFlowGrayscaleMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned OpticalFlowGrayscaleMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& OpticalFlowGrayscaleMemory::descriptor_set(int index) const
{
        ASSERT(index == 0 || index == 1);
        return m_descriptors.descriptor_set(index);
}

void OpticalFlowGrayscaleMemory::set_src(VkSampler sampler, const vulkan::ImageWithMemory& image)
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, SRC_BINDING, image_info);
        }
}

void OpticalFlowGrayscaleMemory::set_dst(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1)
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, DST_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, DST_BINDING, image_info);
}

//

OpticalFlowGrayscaleConstant::OpticalFlowGrayscaleConstant()
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
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, x);
                entry.size = sizeof(Data::x);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, y);
                entry.size = sizeof(Data::y);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, width);
                entry.size = sizeof(Data::width);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, height);
                entry.size = sizeof(Data::height);
                m_entries.push_back(entry);
        }
}

void OpticalFlowGrayscaleConstant::set(uint32_t local_size_x, uint32_t local_size_y, int32_t x, int32_t y, int32_t width,
                                       int32_t height)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;

        static_assert(std::is_same_v<decltype(m_data.x), decltype(x)>);
        m_data.x = x;
        static_assert(std::is_same_v<decltype(m_data.y), decltype(y)>);
        m_data.y = y;
        static_assert(std::is_same_v<decltype(m_data.width), decltype(width)>);
        m_data.width = width;
        static_assert(std::is_same_v<decltype(m_data.height), decltype(height)>);
        m_data.height = height;
}

const std::vector<VkSpecializationMapEntry>& OpticalFlowGrayscaleConstant::entries() const
{
        return m_entries;
}

const void* OpticalFlowGrayscaleConstant::data() const
{
        return &m_data;
}

size_t OpticalFlowGrayscaleConstant::size() const
{
        return sizeof(m_data);
}

//

OpticalFlowGrayscaleProgram::OpticalFlowGrayscaleProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, OpticalFlowGrayscaleMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {OpticalFlowGrayscaleMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, optical_flow_grayscale_comp(), "main")
{
}

VkDescriptorSetLayout OpticalFlowGrayscaleProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout OpticalFlowGrayscaleProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline OpticalFlowGrayscaleProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void OpticalFlowGrayscaleProgram::create_pipeline(uint32_t local_size_x, uint32_t local_size_y, int32_t x, int32_t y,
                                                  int32_t width, int32_t height)
{
        m_constant.set(local_size_x, local_size_y, x, y, width, height);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void OpticalFlowGrayscaleProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
