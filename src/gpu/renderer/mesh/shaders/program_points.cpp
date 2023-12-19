/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "program_points.h"

#include "descriptors.h"
#include "vertex_points.h"

#include <src/com/error.h>
#include <src/numerical/region.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/graphics.h>
#include <src/vulkan/shader.h>
#include <src/vulkan/strings.h>

#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_shared_bindings()
{
        SharedMemory::Flags flags{};

        flags.shadow_matrices = 0;
        flags.drawing = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.objects = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.shadow_map = 0;
        flags.acceleration_structure = 0;
        flags.ggx_f1_albedo = 0;
        flags.transparency = VK_SHADER_STAGE_FRAGMENT_BIT;

        return SharedMemory::descriptor_set_layout_bindings(flags);
}

std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

PointsProgram::PointsProgram(const vulkan::Device* const device, const Code& code)
        : device_(device),
          descriptor_set_layout_shared_(
                  vulkan::create_descriptor_set_layout(device->handle(), descriptor_set_layout_shared_bindings())),
          descriptor_set_layout_mesh_(
                  vulkan::create_descriptor_set_layout(device->handle(), descriptor_set_layout_mesh_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  device->handle(),
                  {SharedMemory::set_number(), MeshMemory::set_number()},
                  {descriptor_set_layout_shared_, descriptor_set_layout_mesh_},
                  push_constant_ranges())),
          vertex_shader_0d_(device_->handle(), code.mesh_points_0d_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          vertex_shader_1d_(device_->handle(), code.mesh_points_1d_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          fragment_shader_(device_->handle(), code.mesh_points_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_shared() const
{
        return descriptor_set_layout_shared_;
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_mesh() const
{
        return descriptor_set_layout_mesh_;
}

VkPipelineLayout PointsProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

const vulkan::Shader* PointsProgram::topology_shader(const VkPrimitiveTopology primitive_topology) const
{
        if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
        {
                return &vertex_shader_0d_;
        }
        if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        {
                return &vertex_shader_1d_;
        }
        error("Unsupported primitive topology " + vulkan::primitive_topology_to_string(primitive_topology)
              + " for renderer points program");
}

vulkan::handle::Pipeline PointsProgram::create_pipeline(
        const vulkan::RenderPass& render_pass,
        const VkSampleCountFlagBits sample_count,
        const VkPrimitiveTopology primitive_topology,
        const Region<2, int>& viewport,
        const bool transparency) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = primitive_topology;
        info.depth_write = !transparency;
        info.shaders = {topology_shader(primitive_topology), &fragment_shader_};
        info.binding_descriptions = PointsVertex::binding_descriptions();
        info.attribute_descriptions = PointsVertex::attribute_descriptions();

        return vulkan::create_graphics_pipeline(info);
}
}
