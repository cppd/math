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

#include "renderer.h"

#include "sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
{
VolumeRenderer::VolumeRenderer(
        const vulkan::Device* const device,
        const Code& code,
        const bool sample_shading,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::Buffer& drawing_buffer,
        const GgxF1Albedo& ggx_f1_albedo)
        : device_(*device),
          sample_shading_(sample_shading),
          //
          coordinates_buffer_(*device, graphics_family_indices),
          //
          volume_program_(device, code),
          //
          shared_memory_(
                  *device,
                  volume_program_.descriptor_set_layout_shared(),
                  volume_program_.descriptor_set_layout_shared_bindings()),
          //
          image_sampler_(create_volume_image_sampler(*device)),
          depth_sampler_(create_volume_depth_image_sampler(*device)),
          transfer_function_sampler_(create_volume_transfer_function_sampler(*device))
{
        shared_memory_.set_drawing(drawing_buffer);
        shared_memory_.set_ggx_f1_albedo(
                ggx_f1_albedo.sampler(), ggx_f1_albedo.cosine_roughness(), ggx_f1_albedo.cosine_weighted_average());
        shared_memory_.set_coordinates(coordinates_buffer_.buffer());
}

void VolumeRenderer::create_buffers(
        const RenderBuffers3D* const render_buffers,
        const Region<2, int>& viewport,
        const VkImageView depth_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::Buffer& transparency_nodes,
        const std::vector<vulkan::ImageWithMemory>& opacity_images)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_buffers();

        render_buffers_ = render_buffers;

        shared_memory_.set_depth_image(depth_image, depth_sampler_);
        shared_memory_.set_transparency(transparency_heads_image.image_view(), transparency_nodes);

        ASSERT(opacity_images.size() == 2);
        shared_memory_.set_opacity(opacity_images[0].image_view(), opacity_images[1].image_view());

        pipeline_image_ = volume_program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgramPipelineType::IMAGE);

        pipeline_image_fragments_ = volume_program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgramPipelineType::IMAGE_FRAGMENTS);

        pipeline_fragments_ = volume_program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgramPipelineType::FRAGMENTS);
}

void VolumeRenderer::delete_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        command_buffers_image_.reset();
        command_buffers_image_fragments_.reset();
        command_buffers_fragments_.reset();
        pipeline_image_.reset();
        pipeline_image_fragments_.reset();
        pipeline_fragments_.reset();
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> VolumeRenderer::image_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(
                volume_program_.descriptor_set_layout_image(), volume_program_.descriptor_set_layout_image_bindings());

        return layouts;
}

VkSampler VolumeRenderer::image_sampler() const
{
        return image_sampler_;
}

VkSampler VolumeRenderer::transfer_function_sampler() const
{
        return transfer_function_sampler_;
}

void VolumeRenderer::draw_commands_fragments(const VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_fragments_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared(),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::draw_commands_image(const VolumeObject* const volume, const VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_image_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared_image(),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared_image(),
                VolumeImageMemory::set_number(), 1 /*set count*/,
                &volume->descriptor_set(volume_program_.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::draw_commands_image_fragments(
        const VolumeObject* const volume,
        const VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_image_fragments_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared_image(),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared_image(),
                VolumeImageMemory::set_number(), 1 /*set count*/,
                &volume->descriptor_set(volume_program_.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::create_command_buffers_fragments(const VkCommandPool graphics_command_pool)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        ASSERT(render_buffers_);

        command_buffers_fragments_.reset();

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                draw_commands_fragments(command_buffer);
        };

        command_buffers_fragments_ = vulkan::create_command_buffers(info);
}

void VolumeRenderer::create_command_buffers(const VkCommandPool graphics_command_pool)
{
        create_command_buffers(nullptr, graphics_command_pool, nullptr);
}

void VolumeRenderer::create_command_buffers(
        const VolumeObject* const volume,
        const VkCommandPool graphics_command_pool,
        const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        ASSERT(render_buffers_);

        delete_command_buffers();

        create_command_buffers_fragments(graphics_command_pool);

        if (!volume)
        {
                return;
        }

        ASSERT(before_render_pass_commands);

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;

        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                draw_commands_image(volume, command_buffer);
        };
        command_buffers_image_ = vulkan::create_command_buffers(info);

        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                draw_commands_image_fragments(volume, command_buffer);
        };
        command_buffers_image_fragments_ = vulkan::create_command_buffers(info);
}

void VolumeRenderer::delete_command_buffers()
{
        command_buffers_fragments_.reset();
        command_buffers_image_.reset();
        command_buffers_image_fragments_.reset();
}

void VolumeRenderer::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image)
{
        delete_command_buffers();
        shared_memory_.set_shadow_image(sampler, shadow_image);
}

void VolumeRenderer::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure)
{
        delete_command_buffers();
        shared_memory_.set_acceleration_structure(acceleration_structure);
}

bool VolumeRenderer::has_volume() const
{
        ASSERT(command_buffers_image_.has_value() == command_buffers_image_fragments_.has_value());

        return command_buffers_image_.has_value();
}

std::optional<VkCommandBuffer> VolumeRenderer::command_buffer(const unsigned index, const bool with_fragments) const
{
        if (has_volume())
        {
                if (with_fragments)
                {
                        ASSERT(index < command_buffers_image_fragments_->count());
                        return (*command_buffers_image_fragments_)[index];
                }
                ASSERT(index < command_buffers_image_->count());
                return (*command_buffers_image_)[index];
        }
        if (with_fragments)
        {
                ASSERT(command_buffers_fragments_);

                ASSERT(index < command_buffers_fragments_->count());
                return (*command_buffers_fragments_)[index];
        }
        return std::nullopt;
}

void VolumeRenderer::set_matrix(const Matrix4d& vp_matrix)
{
        const Matrix4d device_to_world = vp_matrix.inverse();
        coordinates_buffer_.set(device_to_world);
}

void VolumeRenderer::set_matrix(const Matrix4d& vp_matrix, const Matrix4d& world_to_shadow_matrix)
{
        const Matrix4d device_to_world = vp_matrix.inverse();
        const Matrix4d device_to_shadow = world_to_shadow_matrix * device_to_world;
        coordinates_buffer_.set(device_to_world, device_to_shadow);
}
}
