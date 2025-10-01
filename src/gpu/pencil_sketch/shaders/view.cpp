/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/gpu/pencil_sketch/code/code.h>
#include <src/numerical/region.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/graphics.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

namespace ns::gpu::pencil_sketch
{
std::vector<VkDescriptorSetLayoutBinding> ViewMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(1);

        bindings.push_back(
                {.binding = IMAGE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

ViewMemory::ViewMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned ViewMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& ViewMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void ViewMemory::set_image(const VkSampler sampler, const vulkan::ImageView& image) const
{
        ASSERT(image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));

        descriptors_.update_descriptor_set(
                0, IMAGE_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = image.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
}

//

std::vector<VkVertexInputBindingDescription> ViewVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;
        descriptions.reserve(1);

        descriptions.push_back({.binding = 0, .stride = sizeof(ViewVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> ViewVertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;
        descriptions.reserve(2);

        descriptions.push_back(
                {.location = 0,
                 .binding = 0,
                 .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                 .offset = offsetof(ViewVertex, position)});

        descriptions.push_back(
                {.location = 1,
                 .binding = 0,
                 .format = VK_FORMAT_R32G32_SFLOAT,
                 .offset = offsetof(ViewVertex, texture_coordinates)});

        return descriptions;
}

//

ViewProgram::ViewProgram(const vulkan::Device* const device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device->handle(), ViewMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(
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
        const numerical::Region<2, int>& viewport) const
{
        vulkan::pipeline::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        info.shaders = {&vertex_shader_, &fragment_shader_};
        info.binding_descriptions = ViewVertex::binding_descriptions();
        info.attribute_descriptions = ViewVertex::attribute_descriptions();

        return vulkan::pipeline::create_graphics_pipeline(info);
}
}
