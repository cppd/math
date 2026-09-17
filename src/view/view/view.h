/*
Copyright (C) 2017-2026 Topological Manifold

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

#pragma once

#include "clear_buffer.h"
#include "image_process.h"
#include "image_resolve.h"
#include "render_buffers.h"
#include "swapchain.h"
#include "view_info.h"
#include "view_process.h"

#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/view/com/camera.h>
#include <src/view/com/clip_plane.h>
#include <src/view/com/frame_rate.h>
#include <src/view/com/mouse.h>
#include <src/view/event.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device_graphics.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/swapchain.h>
#include <src/window/handle.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::view::view
{
class View final
{
        const std::thread::id thread_id_;

        const vulkan::handle::SurfaceKHR surface_;
        const vulkan::device::DeviceGraphics device_graphics_;
        const vulkan::CommandPool graphics_compute_command_pool_;
        const vulkan::CommandPool compute_command_pool_;
        const vulkan::CommandPool transfer_command_pool_;
        const vulkan::handle::Semaphore swapchain_image_semaphore_;

        VkSampleCountFlagBits sample_count_flag_;

        std::optional<PixelSizes> pixel_sizes_;
        com::FrameRate frame_rate_;

        ClearBuffer clear_buffer_;
        std::unique_ptr<gpu::renderer::Renderer> renderer_;
        std::unique_ptr<gpu::text_writer::View> text_;

        ImageProcess image_process_;
        com::Camera camera_;
        com::Mouse mouse_;
        com::ClipPlane clip_plane_;
        ViewProcess view_process_;

        std::optional<vulkan::Swapchain> swapchain_;
        std::unique_ptr<RenderBuffers> render_buffers_;
        std::optional<vulkan::ImageWithMemory> object_image_;
        std::optional<ImageResolve> image_resolve_;
        std::optional<Swapchain> swapchain_resolve_;

        std::chrono::steady_clock::time_point last_frame_time_;

        //

        void cmd(const ViewCommand& command);
        void cmd(const MouseCommand& command);
        void cmd(const ImageCommand& command);
        void cmd(const ClipPlaneCommand& command);

        void info(std::optional<info::Camera>* camera) const;
        void info(std::optional<info::Image>* image);
        void info(std::optional<info::ClipPlane>* clip_plane);
        void info(std::optional<info::Functionality>* functionality) const;
        void info(std::optional<info::Description>* description) const;
        void info(std::optional<info::SampleCount>* sample_count) const;

        //

        void delete_buffers();
        void create_buffers(VkFormat format, unsigned width, unsigned height);

        [[nodiscard]] VkSemaphore draw() const;

        //

        void delete_swapchain_buffers();
        void create_swapchain_buffers();

        void delete_swapchain();
        void create_swapchain(const std::optional<std::array<double, 2>>& window_size_in_mm = std::nullopt);

        [[nodiscard]] bool render_swapchain() const;

        void set_sample_count(int sample_count);

public:
        View(window::WindowID window, const std::array<double, 2>& window_size_in_mm);

        ~View();

        View(const View&) = delete;
        View(View&&) = delete;
        View& operator=(const View&) = delete;
        View& operator=(View&&) = delete;

        void render();

        void exec(const std::vector<Command>& commands);
        void receive(const std::vector<Info>& infos);
};
}
