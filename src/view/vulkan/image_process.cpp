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

#include "image_process.h"

#include "image_resolve.h"

#include "../event.h"

#include <src/com/error.h>
#include <src/gpu/convex_hull/view.h>
#include <src/gpu/dft/view.h>
#include <src/gpu/optical_flow/view.h>
#include <src/gpu/pencil_sketch/view.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <optional>
#include <variant>

namespace ns::view
{
vulkan::DeviceFunctionality ImageProcess::device_functionality()
{
        vulkan::DeviceFunctionality res;
        res.merge(gpu::convex_hull::View::device_functionality());
        res.merge(gpu::dft::View::device_functionality());
        res.merge(gpu::optical_flow::View::device_functionality());
        res.merge(gpu::pencil_sketch::View::device_functionality());
        return res;
}

ImageProcess::ImageProcess(
        const bool sample_shading,
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const unsigned image_count)
{
        convex_hull_ = gpu::convex_hull::create_view(device, graphics_command_pool, graphics_queue, sample_shading);

        pencil_sketch_ = gpu::pencil_sketch::create_view(device, graphics_command_pool, graphics_queue);

        dft_ = gpu::dft::create_view(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue);

        optical_flow_ = gpu::optical_flow::create_view(
                device, graphics_command_pool, graphics_queue, compute_command_pool, compute_queue);

        resolve_semaphores_.resize(image_count);
        for (std::array<vulkan::handle::Semaphore, 2>& semaphores : resolve_semaphores_)
        {
                semaphores[0] = vulkan::handle::Semaphore(device->handle());
                semaphores[1] = vulkan::handle::Semaphore(device->handle());
        }
}

void ImageProcess::cmd(const command::PencilSketchShow& v)
{
        if (pencil_sketch_active_ != v.show)
        {
                pencil_sketch_active_ = v.show;
        }
}

void ImageProcess::cmd(const command::DftShow& v)
{
        if (dft_active_ != v.show)
        {
                dft_active_ = v.show;
        }
}

void ImageProcess::cmd(const command::DftSetBrightness& v)
{
        dft_->set_brightness(v.value);
}

void ImageProcess::cmd(const command::DftSetBackgroundColor& v)
{
        dft_->set_background_color(v.value);
}

void ImageProcess::cmd(const command::DftSetColor& v)
{
        dft_->set_color(v.value);
}

void ImageProcess::cmd(const command::ConvexHullShow& v)
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

void ImageProcess::cmd(const command::OpticalFlowShow& v)
{
        if (optical_flow_active_ != v.show)
        {
                optical_flow_active_ = v.show;
        }
}

void ImageProcess::exec(const ImageCommand& command)
{
        std::visit(
                [this](const auto& v)
                {
                        cmd(v);
                },
                command);
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
        const double window_ppi,
        gpu::RenderBuffers2D* const render_buffers,
        const vulkan::ImageWithMemory& input,
        const vulkan::ImageWithMemory& objects,
        const Region<2, int>& window_1,
        const std::optional<Region<2, int>>& window_2)
{
        convex_hull_->create_buffers(render_buffers, objects, window_1);

        pencil_sketch_->create_buffers(render_buffers, input, objects, window_1);

        optical_flow_->create_buffers(render_buffers, input, window_ppi, window_1);

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
                ASSERT(image_index < resolve_semaphores_.size());
                const VkSemaphore resolve_semaphore = resolve_semaphores_[image_index][0];
                image_resolve.resolve(graphics_queue, semaphore, resolve_semaphore, image_index);
                semaphore = pencil_sketch_->draw(graphics_queue, resolve_semaphore, image_index);
        }

        if (dft_active_ || optical_flow_active_)
        {
                ASSERT(image_index < resolve_semaphores_.size());
                const VkSemaphore resolve_semaphore = resolve_semaphores_[image_index][1];
                image_resolve.resolve(graphics_queue, semaphore, resolve_semaphore, image_index);
                semaphore = resolve_semaphore;
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
