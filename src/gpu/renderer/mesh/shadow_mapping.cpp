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

#include "shadow_mapping.h"

#include "commands.h"
#include "depth_buffers.h"
#include "object.h"
#include "sampler.h"

#include "buffers/shadow_matrices.h"
#include "shaders/descriptors.h"
#include "shaders/program_shadow.h"

#include <src/com/error.h>
#include <src/gpu/renderer/code/code.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
ShadowMapping::ShadowMapping(
        const vulkan::Device* const device,
        const Code& code,
        const vulkan::Buffer& drawing_buffer,
        const std::vector<std::uint32_t>& drawing_family_indices)
        : triangles_program_(device, code),
          triangles_shared_memory_(
                  device->handle(),
                  triangles_program_.descriptor_set_layout_shared(),
                  triangles_program_.descriptor_set_layout_shared_bindings(),
                  drawing_buffer),
          sampler_(create_mesh_shadow_sampler(device->handle())),
          shadow_matrices_buffer_(*device, drawing_family_indices)
{
        ASSERT(!code.ray_tracing());

        triangles_shared_memory_.set_shadow_matrices(shadow_matrices_buffer_.buffer());
}

void ShadowMapping::create_buffers(
        const unsigned buffer_count,
        const std::vector<std::uint32_t>& family_indices,
        const VkCommandPool graphics_command_pool,
        const VkQueue graphics_queue,
        const vulkan::Device& device,
        const unsigned width,
        const unsigned height,
        const double zoom)
{
        delete_buffers();

        buffers_ = renderer::create_depth_buffers(
                buffer_count, family_indices, graphics_command_pool, graphics_queue, device, width, height, zoom);

        render_triangles_pipeline_ = triangles_program_.create_pipeline(
                buffers_->render_pass(), buffers_->sample_count(),
                numerical::Region<2, int>({0, 0}, {buffers_->width(), buffers_->height()}));
}

void ShadowMapping::delete_buffers()
{
        delete_command_buffers();

        render_triangles_pipeline_.reset();
        buffers_.reset();
}

void ShadowMapping::create_command_buffers(
        const VkDevice device,
        const std::vector<const MeshObject*>& meshes,
        const VkCommandPool graphics_command_pool)
{
        ASSERT(buffers_);

        delete_command_buffers();

        vulkan::CommandBufferCreateInfo info;

        info.device = device;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = buffers_->width();
        info.render_area->extent.height = buffers_->height();
        info.render_pass = buffers_->render_pass().handle();
        info.framebuffers = &buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.clear_values = &buffers_->clear_values();

        if (!meshes.empty())
        {
                info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
                {
                        vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);

                        commands_shadow(
                                meshes, command_buffer, *render_triangles_pipeline_, triangles_program_,
                                triangles_shared_memory_);
                };
        }

        render_command_buffers_ = vulkan::create_command_buffers(info);
}

void ShadowMapping::delete_command_buffers()
{
        render_command_buffers_.reset();
}

void ShadowMapping::set_shadow_matrices(
        const numerical::Matrix4d& vp_matrix,
        const numerical::Matrix4d& world_to_shadow) const
{
        shadow_matrices_buffer_.set(vp_matrix, world_to_shadow);
}

const vulkan::ImageView& ShadowMapping::image_view() const
{
        return buffers_->image_view(0);
}

VkSampler ShadowMapping::sampler() const
{
        return sampler_;
}

const vulkan::Buffer& ShadowMapping::shadow_matrices_buffer() const
{
        return shadow_matrices_buffer_.buffer();
}

VkDescriptorSetLayout ShadowMapping::descriptor_set_layout_mesh() const
{
        return triangles_program_.descriptor_set_layout_mesh();
}

std::vector<VkDescriptorSetLayoutBinding> ShadowMapping::descriptor_set_layout_mesh_bindings() const
{
        return triangles_program_.descriptor_set_layout_mesh_bindings();
}

VkCommandBuffer ShadowMapping::command_buffer(const unsigned index) const
{
        ASSERT(render_command_buffers_);
        ASSERT(index < render_command_buffers_->count());
        return (*render_command_buffers_)[index];
}
}
