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

#include "renderer.h"

#include "sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
{
namespace
{
std::vector<const vulkan::ImageView*> opacity_images(const Opacity& opacity)
{
        std::vector<const vulkan::ImageView*> res(opacity.images().size());
        for (std::size_t i = 0; i < res.size(); ++i)
        {
                res[i] = &opacity.images()[i].image_view();
        }
        return res;
}
}

VolumeRenderer::VolumeRenderer(
        const vulkan::Device* const device,
        const Code& code,
        const bool sample_shading,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::Buffer& drawing_buffer,
        const GgxF1Albedo& ggx_f1_albedo)
        : device_(device->handle()),
          sample_shading_(sample_shading),
          //
          coordinates_buffer_(*device, graphics_family_indices),
          //
          volume_program_(device, code),
          //
          shared_memory_(
                  device_,
                  volume_program_.descriptor_set_layout_shared(),
                  volume_program_.descriptor_set_layout_shared_bindings()),
          //
          image_sampler_(create_volume_image_sampler(device_)),
          depth_sampler_(create_volume_depth_image_sampler(device_)),
          transfer_function_sampler_(create_volume_transfer_function_sampler(device_))
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
        const Opacity& opacity)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_buffers();

        render_buffers_ = render_buffers;

        shared_memory_.set_depth_image(depth_image, depth_sampler_);
        shared_memory_.set_transparency(transparency_heads_image.image_view(), transparency_nodes);
        shared_memory_.set_opacity(opacity_images(opacity));

        const auto create_pipeline = [&](const VolumeProgramPipelineType type)
        {
                pipelines_[type] = volume_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport, type);
        };

        create_pipeline(VolumeProgramPipelineType::IMAGE);
        create_pipeline(VolumeProgramPipelineType::IMAGE_OPACITY);
        create_pipeline(VolumeProgramPipelineType::IMAGE_OPACITY_TRANSPARENCY);
        create_pipeline(VolumeProgramPipelineType::IMAGE_TRANSPARENCY);
        create_pipeline(VolumeProgramPipelineType::OPACITY);
        create_pipeline(VolumeProgramPipelineType::OPACITY_TRANSPARENCY);
        create_pipeline(VolumeProgramPipelineType::TRANSPARENCY);
}

void VolumeRenderer::delete_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        commands_fragments_.reset();
        commands_image_.reset();
        pipelines_.clear();
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

void VolumeRenderer::draw_commands_fragments(const VolumeProgramPipelineType type, const VkCommandBuffer command_buffer)
        const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(type == VolumeProgramPipelineType::OPACITY || type == VolumeProgramPipelineType::OPACITY_TRANSPARENCY
               || type == VolumeProgramPipelineType::TRANSPARENCY);

        const auto pipeline_iter = pipelines_.find(type);
        ASSERT(pipeline_iter != pipelines_.cend());

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_iter->second);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, volume_program_.pipeline_layout_shared(),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::draw_commands_image(
        const VolumeProgramPipelineType type,
        const VolumeObject* const volume,
        const VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(type == VolumeProgramPipelineType::IMAGE || type == VolumeProgramPipelineType::IMAGE_OPACITY
               || type == VolumeProgramPipelineType::IMAGE_OPACITY_TRANSPARENCY
               || type == VolumeProgramPipelineType::IMAGE_TRANSPARENCY);

        const auto pipeline_iter = pipelines_.find(type);
        ASSERT(pipeline_iter != pipelines_.cend());

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_iter->second);

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

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass().handle();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;

        const auto create_buffers =
                [&](const VolumeProgramPipelineType type, vulkan::handle::CommandBuffers* const commands)
        {
                info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
                {
                        draw_commands_fragments(type, command_buffer);
                };
                *commands = vulkan::create_command_buffers(info);
                info.render_pass_commands = nullptr;
        };

        commands_fragments_.emplace();
        create_buffers(VolumeProgramPipelineType::OPACITY, &commands_fragments_->opacity);
        create_buffers(VolumeProgramPipelineType::OPACITY_TRANSPARENCY, &commands_fragments_->opacity_transparency);
        create_buffers(VolumeProgramPipelineType::TRANSPARENCY, &commands_fragments_->transparency);
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
        info.render_pass = render_buffers_->render_pass().handle();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.before_render_pass_commands = before_render_pass_commands;

        const auto create_buffers =
                [&](const VolumeProgramPipelineType type, vulkan::handle::CommandBuffers* const commands)
        {
                info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
                {
                        draw_commands_image(type, volume, command_buffer);
                };
                *commands = vulkan::create_command_buffers(info);
        };

        commands_image_.emplace();
        create_buffers(VolumeProgramPipelineType::IMAGE, &commands_image_->image);
        create_buffers(VolumeProgramPipelineType::IMAGE_OPACITY, &commands_image_->image_opacity);
        create_buffers(
                VolumeProgramPipelineType::IMAGE_OPACITY_TRANSPARENCY, &commands_image_->image_opacity_transparency);
        create_buffers(VolumeProgramPipelineType::IMAGE_TRANSPARENCY, &commands_image_->image_transparency);
}

void VolumeRenderer::delete_command_buffers()
{
        commands_fragments_.reset();
        commands_image_.reset();
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
        return commands_image_.has_value();
}

std::optional<VkCommandBuffer> VolumeRenderer::command_buffer(
        const unsigned index,
        const bool opacity,
        const bool transparency) const
{
        if (has_volume())
        {
                ASSERT(commands_image_);
                if (opacity)
                {
                        if (transparency)
                        {
                                return commands_image_->image_opacity_transparency[index];
                        }
                        return commands_image_->image_opacity[index];
                }
                if (transparency)
                {
                        return commands_image_->image_transparency[index];
                }
                return commands_image_->image[index];
        }
        if (opacity)
        {
                ASSERT(commands_fragments_);
                if (transparency)
                {
                        return commands_fragments_->opacity_transparency[index];
                }
                return commands_fragments_->opacity[index];
        }
        if (transparency)
        {
                ASSERT(commands_fragments_);
                return commands_fragments_->transparency[index];
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
        coordinates_buffer_.set(device_to_world, device_to_shadow, world_to_shadow_matrix);
}
}
