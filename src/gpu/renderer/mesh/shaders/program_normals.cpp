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

#include "program_normals.h"

#include "descriptors.h"
#include "vertex_triangles.h"

#include <src/gpu/renderer/code/code.h>
#include <src/numerical/region.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/graphics.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> NormalsProgram::descriptor_set_layout_shared_bindings()
{
        SharedMemory::Flags flags{};

        flags.shadow_matrices = 0;
        flags.drawing = VK_SHADER_STAGE_GEOMETRY_BIT;
        flags.objects = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.shadow_map = 0;
        flags.acceleration_structure = 0;
        flags.ggx_f1_albedo = 0;
        flags.transparency = VK_SHADER_STAGE_FRAGMENT_BIT;

        return SharedMemory::descriptor_set_layout_bindings(flags);
}

std::vector<VkDescriptorSetLayoutBinding> NormalsProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_GEOMETRY_BIT);
}

NormalsProgram::NormalsProgram(const vulkan::Device* const device, const Code& code)
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
          vertex_shader_(device_->handle(), code.mesh_normals_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          geometry_shader_(device_->handle(), code.mesh_normals_geom(), VK_SHADER_STAGE_GEOMETRY_BIT),
          fragment_shader_(device_->handle(), code.mesh_normals_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout NormalsProgram::descriptor_set_layout_shared() const
{
        return descriptor_set_layout_shared_;
}

VkDescriptorSetLayout NormalsProgram::descriptor_set_layout_mesh() const
{
        return descriptor_set_layout_mesh_;
}

VkPipelineLayout NormalsProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline NormalsProgram::create_pipeline(
        const vulkan::RenderPass& render_pass,
        const VkSampleCountFlagBits sample_count,
        const bool sample_shading,
        const numerical::Region<2, int>& viewport,
        const bool transparency) const
{
        vulkan::pipeline::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        info.depth_write = !transparency;
        info.shaders = {&vertex_shader_, &geometry_shader_, &fragment_shader_};
        info.binding_descriptions = TrianglesVertex::binding_descriptions();
        info.attribute_descriptions = TrianglesVertex::attribute_descriptions_normals();

        return vulkan::pipeline::create_graphics_pipeline(info);
}
}
