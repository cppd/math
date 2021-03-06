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

#include "grayscale.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::optical_flow
{
std::vector<VkDescriptorSetLayoutBinding> GrayscaleMemory::descriptor_set_layout_bindings()
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

GrayscaleMemory::GrayscaleMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned GrayscaleMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& GrayscaleMemory::descriptor_set(int index) const
{
        ASSERT(index == 0 || index == 1);
        return m_descriptors.descriptor_set(index);
}

void GrayscaleMemory::set_src(VkSampler sampler, const vulkan::ImageWithMemory& image)
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image.image_view();
        image_info.sampler = sampler;

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, SRC_BINDING, image_info);
        }
}

void GrayscaleMemory::set_dst(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1)
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_0.format() == VK_FORMAT_R32_SFLOAT);
        ASSERT(image_1.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(image_1.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, DST_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, DST_BINDING, image_info);
}

//

GrayscaleConstant::GrayscaleConstant()
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

void GrayscaleConstant::set(uint32_t local_size_x, uint32_t local_size_y, const Region<2, int>& rectangle)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;

        ASSERT(rectangle.is_positive());
        m_data.x = rectangle.x0();
        m_data.y = rectangle.y0();
        m_data.width = rectangle.width();
        m_data.height = rectangle.height();
}

const std::vector<VkSpecializationMapEntry>& GrayscaleConstant::entries() const
{
        return m_entries;
}

const void* GrayscaleConstant::data() const
{
        return &m_data;
}

std::size_t GrayscaleConstant::size() const
{
        return sizeof(m_data);
}

//

GrayscaleProgram::GrayscaleProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, GrayscaleMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {GrayscaleMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_grayscale_comp(), "main")
{
}

VkDescriptorSetLayout GrayscaleProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout GrayscaleProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline GrayscaleProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void GrayscaleProgram::create_pipeline(uint32_t local_size_x, uint32_t local_size_y, const Region<2, int>& rectangle)
{
        m_constant.set(local_size_x, local_size_y, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void GrayscaleProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
