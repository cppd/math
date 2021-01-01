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

#include "fft_global.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::dft
{
std::vector<VkDescriptorSetLayoutBinding> FftGlobalMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

FftGlobalMemory::FftGlobalMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Data));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DATA_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned FftGlobalMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& FftGlobalMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void FftGlobalMemory::set_data(float two_pi_div_m, int m_div_2) const
{
        Data d;
        d.two_pi_div_m = two_pi_div_m;
        d.m_div_2 = m_div_2;
        vulkan::map_and_write_to_buffer(m_uniform_buffers[0], d);
}

void FftGlobalMemory::set_buffer(const vulkan::BufferWithMemory& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        m_descriptors.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
}

//

FftGlobalConstant::FftGlobalConstant()
{
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 0;
                entry.offset = offsetof(Data, group_size);
                entry.size = sizeof(Data::group_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 1;
                entry.offset = offsetof(Data, inverse);
                entry.size = sizeof(Data::inverse);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 2;
                entry.offset = offsetof(Data, data_size);
                entry.size = sizeof(Data::data_size);
                m_entries.push_back(entry);
        }
        {
                VkSpecializationMapEntry entry = {};
                entry.constantID = 3;
                entry.offset = offsetof(Data, n);
                entry.size = sizeof(Data::n);
                m_entries.push_back(entry);
        }
}

void FftGlobalConstant::set(uint32_t group_size, bool inverse, uint32_t data_size, uint32_t n)
{
        static_assert(std::is_same_v<decltype(m_data.group_size), decltype(group_size)>);
        m_data.group_size = group_size;
        static_assert(std::is_same_v<decltype(m_data.inverse), uint32_t>);
        m_data.inverse = inverse ? 1 : 0;
        static_assert(std::is_same_v<decltype(m_data.data_size), decltype(data_size)>);
        m_data.data_size = data_size;
        static_assert(std::is_same_v<decltype(m_data.n), decltype(n)>);
        m_data.n = n;
}

const std::vector<VkSpecializationMapEntry>& FftGlobalConstant::entries() const
{
        return m_entries;
}

const void* FftGlobalConstant::data() const
{
        return &m_data;
}

std::size_t FftGlobalConstant::size() const
{
        return sizeof(m_data);
}

//

FftGlobalProgram::FftGlobalProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, FftGlobalMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {FftGlobalMemory::set_number()}, {m_descriptor_set_layout})),
          m_shader(device, code_fft_global_comp(), "main")
{
}

VkDescriptorSetLayout FftGlobalProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout FftGlobalProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

VkPipeline FftGlobalProgram::pipeline(bool inverse) const
{
        if (inverse)
        {
                ASSERT(m_pipeline_inverse != VK_NULL_HANDLE);
                return m_pipeline_inverse;
        }

        ASSERT(m_pipeline_forward != VK_NULL_HANDLE);
        return m_pipeline_forward;
}

void FftGlobalProgram::create_pipelines(uint32_t group_size, uint32_t data_size, uint32_t n)
{
        {
                m_constant.set(group_size, false, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline_forward = create_compute_pipeline(info);
        }
        {
                m_constant.set(group_size, true, data_size, n);

                vulkan::ComputePipelineCreateInfo info;
                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_shader;
                info.constants = &m_constant;
                m_pipeline_inverse = create_compute_pipeline(info);
        }
}

void FftGlobalProgram::delete_pipelines()
{
        m_pipeline_forward = vulkan::Pipeline();
        m_pipeline_inverse = vulkan::Pipeline();
}
}
