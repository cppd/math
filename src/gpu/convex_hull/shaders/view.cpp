/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/graphics.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

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
        const decltype(Data().matrix) m = vulkan::to_std140<float>(matrix);
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, matrix), m);
}

void ViewDataBuffer::set_brightness(const float brightness) const
{
        const decltype(Data().brightness) b = brightness;
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Data, brightness), b);
}

//

std::vector<VkDescriptorSetLayoutBinding> ViewMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = DATA_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = POINTS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

ViewMemory::ViewMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        descriptors_.update_descriptor_set(
                0, DATA_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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

        descriptors_.update_descriptor_set(
                0, POINTS_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
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
        info.shaders = {&vertex_shader_, &fragment_shader_};

        return vulkan::create_graphics_pipeline(info);
}
}
