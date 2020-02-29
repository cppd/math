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

#include "compute_flow.h"

#include "shader_source.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> OpticalFlowFlowMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TOP_POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_FLOW_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_FLOW_GUESS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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
                b.binding = J_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

OpticalFlowFlowMemory::OpticalFlowFlowMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 2, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(BufferData));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DATA_BINDING);
        }

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, bindings, infos);
        }
}

unsigned OpticalFlowFlowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& OpticalFlowFlowMemory::descriptor_set(int index) const
{
        ASSERT(index == 0 || index == 1);
        return m_descriptors.descriptor_set(index);
}

void OpticalFlowFlowMemory::set_data(const Data& data) const
{
        BufferData buffer_data;
        buffer_data.point_count_x = data.point_count_x;
        buffer_data.point_count_y = data.point_count_y;
        buffer_data.use_all_points = data.use_all_points ? 1 : 0;
        buffer_data.use_guess = data.use_guess ? 1 : 0;
        buffer_data.guess_kx = data.guess_kx;
        buffer_data.guess_ky = data.guess_ky;
        buffer_data.guess_width = data.guess_width;
        vulkan::map_and_write_to_buffer(m_uniform_buffers[0], buffer_data);
}

void OpticalFlowFlowMemory::set_dx(const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.image_view();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, DX_BINDING, image_info);
        }
}

void OpticalFlowFlowMemory::set_dy(const vulkan::ImageWithMemory& image) const
{
        ASSERT(image.usage() & VK_IMAGE_USAGE_STORAGE_BIT);
        ASSERT(image.format() == VK_FORMAT_R32_SFLOAT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.image_view();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, DY_BINDING, image_info);
        }
}

void OpticalFlowFlowMemory::set_i(const vulkan::ImageWithMemory& image_0, const vulkan::ImageWithMemory& image_1) const
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

void OpticalFlowFlowMemory::set_j(
        VkSampler sampler,
        const vulkan::ImageWithMemory& image_0,
        const vulkan::ImageWithMemory& image_1) const
{
        ASSERT(&image_0 != &image_1);
        ASSERT(image_0.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
        ASSERT(image_1.usage() & VK_IMAGE_USAGE_SAMPLED_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.sampler = sampler;

        image_info.imageView = image_0.image_view();
        m_descriptors.update_descriptor_set(0, J_BINDING, image_info);
        image_info.imageView = image_1.image_view();
        m_descriptors.update_descriptor_set(1, J_BINDING, image_info);
}

void OpticalFlowFlowMemory::set_top_points(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, TOP_POINTS_BINDING, buffer_info);
        }
}

void OpticalFlowFlowMemory::set_flow(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, POINTS_FLOW_BINDING, buffer_info);
        }
}

void OpticalFlowFlowMemory::set_flow_guess(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        for (int s = 0; s < 2; ++s)
        {
                m_descriptors.update_descriptor_set(s, POINTS_FLOW_GUESS_BINDING, buffer_info);
        }
}

//

OpticalFlowFlowConstant::OpticalFlowFlowConstant()
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
                entry.offset = offsetof(Data, radius);
                entry.size = sizeof(Data::radius);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, iteration_count);
                entry.size = sizeof(Data::iteration_count);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 4;
                entry.offset = offsetof(Data, stop_move_square);
                entry.size = sizeof(Data::stop_move_square);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 5;
                entry.offset = offsetof(Data, min_determinant);
                entry.size = sizeof(Data::min_determinant);
                m_entries.push_back(entry);
        }
}

void OpticalFlowFlowConstant::set(
        uint32_t local_size_x,
        uint32_t local_size_y,
        int32_t radius,
        int32_t iteration_count,
        float stop_move_square,
        float min_determinant)
{
        static_assert(std::is_same_v<decltype(m_data.local_size_x), decltype(local_size_x)>);
        m_data.local_size_x = local_size_x;
        static_assert(std::is_same_v<decltype(m_data.local_size_y), decltype(local_size_y)>);
        m_data.local_size_y = local_size_y;
        static_assert(std::is_same_v<decltype(m_data.radius), decltype(radius)>);
        m_data.radius = radius;
        static_assert(std::is_same_v<decltype(m_data.iteration_count), decltype(iteration_count)>);
        m_data.iteration_count = iteration_count;
        static_assert(std::is_same_v<decltype(m_data.stop_move_square), decltype(stop_move_square)>);
        m_data.stop_move_square = stop_move_square;
        static_assert(std::is_same_v<decltype(m_data.min_determinant), decltype(min_determinant)>);
        m_data.min_determinant = min_determinant;
}

const std::vector<VkSpecializationMapEntry>& OpticalFlowFlowConstant::entries() const
{
        return m_entries;
}

const void* OpticalFlowFlowConstant::data() const
{
        return &m_data;
}

size_t OpticalFlowFlowConstant::size() const
{
        return sizeof(m_data);
}

//

OpticalFlowFlowProgram::OpticalFlowFlowProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(vulkan::create_descriptor_set_layout(
                  device,
                  OpticalFlowFlowMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {OpticalFlowFlowMemory::set_number()},
                  {m_descriptor_set_layout})),
          m_shader(device, optical_flow_flow_comp(), "main")
{
}

VkDescriptorSetLayout OpticalFlowFlowProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout OpticalFlowFlowProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline OpticalFlowFlowProgram::pipeline() const
{
        ASSERT(m_pipeline != VK_NULL_HANDLE);
        return m_pipeline;
}

void OpticalFlowFlowProgram::create_pipeline(
        uint32_t local_size_x,
        uint32_t local_size_y,
        int32_t radius,
        int32_t iteration_count,
        float stop_move_square,
        float min_determinant)
{
        m_constant.set(local_size_x, local_size_y, radius, iteration_count, stop_move_square, min_determinant);

        vulkan::ComputePipelineCreateInfo info;
        info.device = &m_device;
        info.pipeline_layout = m_pipeline_layout;
        info.shader = &m_shader;
        info.constants = &m_constant;
        m_pipeline = create_compute_pipeline(info);
}

void OpticalFlowFlowProgram::delete_pipeline()
{
        m_pipeline = vulkan::Pipeline();
}
}
