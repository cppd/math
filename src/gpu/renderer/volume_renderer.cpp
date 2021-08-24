/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "volume_renderer.h"

#include "volume_sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
{
VolumeRenderer::VolumeRenderer(const vulkan::Device& device, bool sample_shading, const ShaderBuffers& buffers)
        : device_(device),
          sample_shading_(sample_shading),
          //
          program_(device),
          shared_memory_(
                  device,
                  program_.descriptor_set_layout_shared(),
                  program_.descriptor_set_layout_shared_bindings(),
                  buffers.drawing_buffer()),
          //
          image_sampler_(create_volume_image_sampler(device_)),
          depth_sampler_(create_volume_depth_image_sampler(device_)),
          transfer_function_sampler_(create_volume_transfer_function_sampler(device_))
{
}

void VolumeRenderer::create_buffers(
        const RenderBuffers3D* render_buffers,
        VkCommandPool graphics_command_pool,
        const Region<2, int>& viewport,
        VkImageView depth_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::Buffer& transparency_nodes)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_buffers();

        render_buffers_ = render_buffers;

        shared_memory_.set_depth_image(depth_image, depth_sampler_);
        shared_memory_.set_transparency(transparency_heads_image, transparency_nodes);

        pipeline_image_ = program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgram::PipelineType::IMAGE);

        pipeline_image_fragments_ = program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgram::PipelineType::IMAGE_FRAGMENTS);

        pipeline_fragments_ = program_.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                VolumeProgram::PipelineType::FRAGMENTS);

        create_command_buffers_fragments(graphics_command_pool);
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

        layouts.emplace_back(program_.descriptor_set_layout_image(), program_.descriptor_set_layout_image_bindings());

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

void VolumeRenderer::draw_commands_fragments(VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_fragments_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                program_.pipeline_layout(VolumeProgram::PipelineLayoutType::FRAGMENTS),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::draw_commands_image(const VolumeObject* volume, VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_image_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                program_.pipeline_layout(VolumeProgram::PipelineLayoutType::IMAGE_FRAGMENTS),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                program_.pipeline_layout(VolumeProgram::PipelineLayoutType::IMAGE_FRAGMENTS),
                VolumeImageMemory::set_number(), 1 /*set count*/,
                &volume->descriptor_set(program_.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::draw_commands_image_fragments(const VolumeObject* volume, VkCommandBuffer command_buffer) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_image_fragments_);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                program_.pipeline_layout(VolumeProgram::PipelineLayoutType::IMAGE_FRAGMENTS),
                VolumeSharedMemory::set_number(), 1 /*set count*/, &shared_memory_.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                program_.pipeline_layout(VolumeProgram::PipelineLayoutType::IMAGE_FRAGMENTS),
                VolumeImageMemory::set_number(), 1 /*set count*/,
                &volume->descriptor_set(program_.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::create_command_buffers_fragments(VkCommandPool graphics_command_pool)
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
        info.render_pass_commands = [&](VkCommandBuffer command_buffer)
        {
                draw_commands_fragments(command_buffer);
        };

        command_buffers_fragments_ = vulkan::create_command_buffers(info);
}

void VolumeRenderer::create_command_buffers(
        const VolumeObject* volume,
        VkCommandPool graphics_command_pool,
        const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        ASSERT(render_buffers_);

        delete_command_buffers();

        if (!volume)
        {
                return;
        }

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

        info.render_pass_commands = [&](VkCommandBuffer command_buffer)
        {
                draw_commands_image(volume, command_buffer);
        };
        command_buffers_image_ = vulkan::create_command_buffers(info);

        info.render_pass_commands = [&](VkCommandBuffer command_buffer)
        {
                draw_commands_image_fragments(volume, command_buffer);
        };
        command_buffers_image_fragments_ = vulkan::create_command_buffers(info);
}

void VolumeRenderer::delete_command_buffers()
{
        command_buffers_image_.reset();
        command_buffers_image_fragments_.reset();
}

bool VolumeRenderer::has_volume() const
{
        ASSERT(command_buffers_image_.has_value() == command_buffers_image_fragments_.has_value());

        return command_buffers_image_.has_value();
}

std::optional<VkCommandBuffer> VolumeRenderer::command_buffer(unsigned index, bool with_fragments) const
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
}
