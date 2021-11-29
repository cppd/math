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

#include "image_process.h"

#include <src/com/error.h>
#include <src/vulkan/features.h>

namespace ns::view
{
vulkan::DeviceFeatures ImageProcess::required_device_features()
{
        vulkan::DeviceFeatures features;
        vulkan::add_features(&features, gpu::convex_hull::View::required_device_features());
        vulkan::add_features(&features, gpu::dft::View::required_device_features());
        vulkan::add_features(&features, gpu::optical_flow::View::required_device_features());
        vulkan::add_features(&features, gpu::pencil_sketch::View::required_device_features());
        return features;
}

ImageProcess::ImageProcess(
        const double window_ppi,
        const bool sample_shading,
        const vulkan::VulkanInstance* const instance,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
        : window_ppi_(window_ppi)
{
        convex_hull_ = gpu::convex_hull::create_view(instance, graphics_command_pool, graphics_queue, sample_shading);

        pencil_sketch_ = gpu::pencil_sketch::create_view(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading);

        dft_ = gpu::dft::create_view(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading);

        optical_flow_ = gpu::optical_flow::create_view(
                instance, graphics_command_pool, graphics_queue, compute_command_pool, compute_queue,
                transfer_command_pool, transfer_queue, sample_shading);
}

void ImageProcess::command(const command::PencilSketchShow& v)
{
        if (pencil_sketch_active_ != v.show)
        {
                pencil_sketch_active_ = v.show;
        }
}

void ImageProcess::command(const command::DftShow& v)
{
        if (dft_active_ != v.show)
        {
                dft_active_ = v.show;
        }
}

void ImageProcess::command(const command::DftSetBrightness& v)
{
        dft_->set_brightness(v.value);
}

void ImageProcess::command(const command::DftSetBackgroundColor& v)
{
        dft_->set_background_color(v.value);
}

void ImageProcess::command(const command::DftSetColor& v)
{
        dft_->set_color(v.value);
}

void ImageProcess::command(const command::ConvexHullShow& v)
{
        if (convex_hull_active_ != v.show)
        {
                convex_hull_active_ = v.show;
                if (convex_hull_active_)
                {
                        convex_hull_->reset_timer();
                }
        }
}

void ImageProcess::command(const command::OpticalFlowShow& v)
{
        if (optical_flow_active_ != v.show)
        {
                optical_flow_active_ = v.show;
        }
}

void ImageProcess::command(const ImageCommand& image_command)
{
        const auto visitor = [this](const auto& v)
        {
                command(v);
        };
        std::visit(visitor, image_command);
}

bool ImageProcess::two_windows() const
{
        return dft_active_;
}

void ImageProcess::delete_buffers()
{
        convex_hull_->delete_buffers();
        pencil_sketch_->delete_buffers();
        dft_->delete_buffers();
        optical_flow_->delete_buffers();
}

void ImageProcess::create_buffers(
        gpu::RenderBuffers2D* const render_buffers,
        const vulkan::ImageWithMemory& input,
        const vulkan::ImageWithMemory& objects,
        const Region<2, int>& window_1,
        const std::optional<Region<2, int>>& window_2)
{
        convex_hull_->create_buffers(render_buffers, objects, window_1);

        pencil_sketch_->create_buffers(render_buffers, input, objects, window_1);

        optical_flow_->create_buffers(render_buffers, input, window_ppi_, window_1);

        if (window_2)
        {
                ASSERT(two_windows());
                dft_->create_buffers(render_buffers, input, window_1, *window_2);
        }
}

VkSemaphore ImageProcess::draw(
        const ImageResolve& image_resolve,
        VkSemaphore semaphore,
        const vulkan::Queue& graphics_queue,
        const vulkan::Queue& compute_queue,
        const unsigned image_index) const
{
        if (pencil_sketch_active_)
        {
                semaphore = image_resolve.resolve_semaphore(graphics_queue, semaphore, image_index);
                semaphore = pencil_sketch_->draw(graphics_queue, semaphore, image_index);
        }

        if (dft_active_ || optical_flow_active_)
        {
                semaphore = image_resolve.resolve_semaphore(graphics_queue, semaphore, image_index);
        }

        if (dft_active_)
        {
                semaphore = dft_->draw(graphics_queue, semaphore, image_index);
        }

        if (optical_flow_active_)
        {
                semaphore = optical_flow_->draw(graphics_queue, compute_queue, semaphore, image_index);
        }

        if (convex_hull_active_)
        {
                semaphore = convex_hull_->draw(graphics_queue, semaphore, image_index);
        }

        return semaphore;
}
}
