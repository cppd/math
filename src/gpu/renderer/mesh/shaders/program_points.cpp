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

#include "program_points.h"

#include "descriptors.h"
#include "vertex_points.h"

#include "../../code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_shared_bindings()
{
        return SharedMemory::descriptor_set_layout_bindings(
                VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                VK_SHADER_STAGE_FRAGMENT_BIT);
}

std::vector<VkDescriptorSetLayoutBinding> PointsProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

PointsProgram::PointsProgram(const vulkan::Device* const device)
        : device_(device),
          descriptor_set_layout_shared_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_shared_bindings())),
          descriptor_set_layout_mesh_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_mesh_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  *device,
                  {SharedMemory::set_number(), MeshMemory::set_number()},
                  {descriptor_set_layout_shared_, descriptor_set_layout_mesh_})),
          vertex_shader_0d_(*device_, code_mesh_points_0d_vert(), "main"),
          vertex_shader_1d_(*device_, code_mesh_points_1d_vert(), "main"),
          fragment_shader_(*device_, code_mesh_points_frag(), "main")
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

vulkan::handle::Pipeline PointsProgram::create_pipeline(
        const VkRenderPass render_pass,
        const VkSampleCountFlagBits sample_count,
        const VkPrimitiveTopology primitive_topology,
        const Region<2, int>& viewport,
        const bool transparency) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = primitive_topology;

        SharedConstants shared_constants;
        shared_constants.set(transparency);
        info.depth_write = !transparency;

        std::vector<const vulkan::Shader*> shaders;
        if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
        {
                shaders = {&vertex_shader_0d_, &fragment_shader_};
        }
        else if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        {
                shaders = {&vertex_shader_1d_, &fragment_shader_};
        }
        else
        {
                error_fatal("Unsupported primitive topology for renderer points program");
        }
        const std::vector<const vulkan::SpecializationConstant*> constants = {&shared_constants, &shared_constants};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions = PointsVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                PointsVertex::attribute_descriptions();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
