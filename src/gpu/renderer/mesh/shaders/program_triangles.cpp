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

#include "program_triangles.h"

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
std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_shared_bindings() const
{
        SharedMemory::Flags flags{};

        flags.shadow_matrices = !ray_tracing_ ? (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) : 0;
        flags.drawing = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.objects = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.shadow_map = !ray_tracing_ ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        flags.acceleration_structure = ray_tracing_ ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        flags.ggx_f1_albedo = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.transparency = VK_SHADER_STAGE_FRAGMENT_BIT;

        return SharedMemory::descriptor_set_layout_bindings(flags);
}

std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_material_bindings()
{
        return MaterialMemory::descriptor_set_layout_bindings();
}

TrianglesProgram::TrianglesProgram(const vulkan::Device* const device, const Code& code)
        : device_(device),
          ray_tracing_(code.ray_tracing()),
          descriptor_set_layout_shared_(
                  vulkan::create_descriptor_set_layout(device->handle(), descriptor_set_layout_shared_bindings())),
          descriptor_set_layout_mesh_(
                  vulkan::create_descriptor_set_layout(device->handle(), descriptor_set_layout_mesh_bindings())),
          descriptor_set_layout_material_(
                  vulkan::create_descriptor_set_layout(device->handle(), descriptor_set_layout_material_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  device->handle(),
                  {SharedMemory::set_number(), MeshMemory::set_number(), MaterialMemory::set_number()},
                  {descriptor_set_layout_shared_, descriptor_set_layout_mesh_, descriptor_set_layout_material_},
                  push_constant_ranges())),
          vertex_shader_(device_->handle(), code.mesh_triangles_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          geometry_shader_(device_->handle(), code.mesh_triangles_geom(), VK_SHADER_STAGE_GEOMETRY_BIT),
          fragment_shader_(device_->handle(), code.mesh_triangles_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_shared() const
{
        return descriptor_set_layout_shared_;
}

VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_mesh() const
{
        return descriptor_set_layout_mesh_;
}

VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_material() const
{
        return descriptor_set_layout_material_;
}

VkPipelineLayout TrianglesProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline TrianglesProgram::create_pipeline(
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
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_write = !transparency;
        info.shaders = {&vertex_shader_, &geometry_shader_, &fragment_shader_};
        info.binding_descriptions = TrianglesVertex::binding_descriptions();
        info.attribute_descriptions = TrianglesVertex::attribute_descriptions_triangles();

        return vulkan::pipeline::create_graphics_pipeline(info);
}
}
