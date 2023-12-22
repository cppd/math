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

#include "image_resolve.h"

#include "render_buffers.h"

#include <src/com/error.h>
#include <src/image/alpha.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/error.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>

#include <cstddef>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::view
{
ImageResolve::ImageResolve(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const RenderBuffers& render_buffers,
        const Region<2, int>& rectangle,
        const VkImageLayout image_layout,
        const VkImageUsageFlags usage)
        : family_index_(command_pool.family_index())
{
        const std::size_t count = render_buffers.image_views().size();

        images_.reserve(count);

        command_buffers_ = vulkan::handle::CommandBuffers(device.handle(), command_pool.handle(), count);

        for (unsigned i = 0; i < count; ++i)
        {
                images_.emplace_back(
                        device, std::vector({command_pool.family_index()}),
                        std::vector({render_buffers.color_format()}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers.width(), render_buffers.height()),
                        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_layout, command_pool, queue);

                const auto commands = [&]()
                {
                        render_buffers.commands_color_resolve(
                                command_buffers_[i], images_[i].image().handle(), image_layout, rectangle, i);
                };

                vulkan::record_commands(command_buffers_[i], commands);
        }
}

const vulkan::ImageWithMemory& ImageResolve::image(const unsigned image_index) const
{
        ASSERT(image_index < images_.size());
        return images_[image_index];
}

void ImageResolve::resolve(
        const vulkan::Queue& graphics_queue,
        const VkSemaphore wait_semaphore,
        const VkSemaphore signal_semaphore,
        const unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == family_index_);
        ASSERT(image_index < command_buffers_.count());

        vulkan::queue_submit(
                wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, command_buffers_[image_index], signal_semaphore,
                graphics_queue.handle());
}

void ImageResolve::resolve(
        const vulkan::Queue& graphics_queue,
        const VkSemaphore wait_semaphore,
        const unsigned image_index) const
{
        ASSERT(graphics_queue.family_index() == family_index_);
        ASSERT(image_index < command_buffers_.count());

        vulkan::queue_submit(
                wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, command_buffers_[image_index],
                graphics_queue.handle());
}

image::Image<2> resolve_to_image(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const RenderBuffers& render_buffers,
        const VkSemaphore wait_semaphore,
        const unsigned image_index)
{
        constexpr VkImageLayout IMAGE_LAYOUT = VK_IMAGE_LAYOUT_GENERAL;

        const unsigned width = render_buffers.width();
        const unsigned height = render_buffers.height();

        const ImageResolve image(
                device, command_pool, queue, render_buffers, Region<2, int>({0, 0}, {width, height}), IMAGE_LAYOUT,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        image.resolve(queue, wait_semaphore, image_index);
        VULKAN_CHECK(vkQueueWaitIdle(queue.handle()));

        image::Image<2> res;

        res.size[0] = width;
        res.size[1] = height;

        image.image(image_index).read(command_pool, queue, IMAGE_LAYOUT, IMAGE_LAYOUT, &res.color_format, &res.pixels);

        ASSERT(4 == image::format_component_count(res.color_format));

        return image::delete_alpha(res);
}
}
