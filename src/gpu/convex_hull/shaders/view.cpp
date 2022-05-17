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

#include "view.h"

#include "../code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::convex_hull
{
ViewDataBuffer::ViewDataBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Data))
{
}

const vulkan::Buffer& ViewDataBuffer::buffer() const
{
        return buffer_.buffer();
}

void ViewDataBuffer::set_matrix(const Matrix4d& matrix) const
{
        decltype(Data().matrix) m = to_matrix<float>(matrix).transpose();
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, matrix), m);
}

void ViewDataBuffer::set_brightness(const float brightness) const
{
        decltype(Data().brightness) b = brightness;
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, brightness), b);
}

//

std::vector<VkDescriptorSetLayoutBinding> ViewMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DATA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = POINTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

ViewMemory::ViewMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& data_buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = data_buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = data_buffer.size();

        descriptors_.update_descriptor_set(0, DATA_BINDING, buffer_info);
}

unsigned ViewMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& ViewMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void ViewMemory::set_points(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, POINTS_BINDING, buffer_info);
}

//

ViewProgram::ViewProgram(const vulkan::Device* const device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device->handle(), ViewMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  device->handle(),
                  {ViewMemory::set_number()},
                  {descriptor_set_layout_})),
          vertex_shader_(device_->handle(), code_view_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          fragment_shader_(device_->handle(), code_view_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout ViewProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout ViewProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline ViewProgram::create_pipeline(
        const vulkan::RenderPass& render_pass,
        const VkSampleCountFlagBits sample_count,
        const bool sample_shading,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

        const std::vector<const vulkan::Shader*> shaders = {&vertex_shader_, &fragment_shader_};
        info.shaders = &shaders;

        return vulkan::create_graphics_pipeline(info);
}
}
