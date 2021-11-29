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

#include "view.h"

#include "image_process.h"
#include "image_resolve.h"
#include "instance.h"
#include "render_buffers.h"
#include "swapchain.h"
#include "view_process.h"

#include "../com/camera.h"
#include "../com/clip_plane.h"
#include "../com/frame_rate.h"
#include "../com/mouse.h"
#include "../com/view_thread.h"
#include "../com/window.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/image/alpha.h>
#include <src/numerical/region.h>
#include <src/vulkan/error.h>
#include <src/vulkan/features.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>

#include <chrono>

namespace ns::view
{
namespace
{
using FrameClock = std::chrono::steady_clock;
constexpr FrameClock::duration IDLE_MODE_FRAME_DURATION = std::chrono::milliseconds(100);

constexpr double FRAME_SIZE_IN_MILLIMETERS = 0.5;

constexpr int RENDER_BUFFER_COUNT = 1;

constexpr int SWAPCHAIN_PREFERRED_IMAGE_COUNT = 2; // 2 - double buffering, 3 - triple buffering
constexpr VkSurfaceFormatKHR SWAPCHAIN_SURFACE_FORMAT{
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
constexpr bool SWAPCHAIN_INITIAL_VERTICAL_SYNC = false;

constexpr VkFormat SAVE_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr std::array DEPTH_FORMATS = std::to_array<VkFormat>({VK_FORMAT_D32_SFLOAT});

constexpr int MINIMUM_SAMPLE_COUNT = 4;
constexpr bool SAMPLE_RATE_SHADING = true; // supersampling
constexpr bool SAMPLER_ANISOTROPY = true; // anisotropic filtering

constexpr VkFormat OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

constexpr color::Color DEFAULT_TEXT_COLOR{RGB8(255, 255, 255)};

static_assert(
        2 <= std::tuple_size_v<
                std::remove_reference_t<decltype(std::declval<vulkan::VulkanInstance>().graphics_compute_queues())>>);

vulkan::DeviceFeatures device_features()
{
        vulkan::DeviceFeatures features{};
        if (MINIMUM_SAMPLE_COUNT > 1 && SAMPLE_RATE_SHADING)
        {
                features.features_10.sampleRateShading = VK_TRUE;
        }
        if (SAMPLER_ANISOTROPY)
        {
                features.features_10.samplerAnisotropy = VK_TRUE;
        }
        vulkan::add_features(&features, gpu::renderer::Renderer::required_device_features());
        vulkan::add_features(&features, gpu::text_writer::View::required_device_features());
        vulkan::add_features(&features, ImageProcess::required_device_features());
        return features;
}

int frame_size_in_pixels(const double window_ppi)
{
        if (!(window_ppi > 0))
        {
                error("Window PPI " + to_string(window_ppi) + "is not positive");
        }
        return std::max(1, millimeters_to_pixels(FRAME_SIZE_IN_MILLIMETERS, window_ppi));
}

class Impl final
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const int frame_size_in_pixels_;

        FrameRate frame_rate_;

        const std::unique_ptr<vulkan::VulkanInstance> instance_;
        const vulkan::handle::Semaphore swapchain_image_semaphore_{instance_->device()};

        std::unique_ptr<gpu::renderer::Renderer> renderer_;
        std::unique_ptr<gpu::text_writer::View> text_;

        ImageProcess image_process_;

        Camera camera_;
        Mouse mouse_;
        ClipPlane clip_plane_;
        ViewProcess view_process_;

        std::optional<vulkan::Swapchain> swapchain_;
        std::unique_ptr<RenderBuffers> render_buffers_;
        std::optional<vulkan::ImageWithMemory> object_image_;
        std::optional<ImageResolve> image_resolve_;
        std::optional<Swapchain> swapchain_resolve_;

        //

        void command(const ViewCommand& view_command)
        {
                view_process_.command(view_command);
        }

        void command(const MouseCommand& mouse_command)
        {
                mouse_.command(mouse_command);
        }

        void command(const ImageCommand& image_command)
        {
                const bool two_windows = image_process_.two_windows();
                image_process_.command(image_command);
                if (two_windows != image_process_.two_windows())
                {
                        create_swapchain();
                }
        }

        void command(const ClipPlaneCommand& clip_plane_command)
        {
                clip_plane_.command(clip_plane_command);
        }

        void info(info::Camera* const camera_info) const
        {
                *camera_info = camera_.view_info();
        }

        void info(info::Image* const image_info)
        {
                static_assert(RENDER_BUFFER_COUNT == 1);

                static constexpr int IMAGE_INDEX = 0;

                instance_->device_wait_idle();

                const int width = swapchain_->width();
                const int height = swapchain_->height();

                delete_swapchain_buffers();
                create_buffers(SAVE_FORMAT, width, height);

                const VkSemaphore semaphore = render();

                image_info->image = resolve_to_image(
                        instance_->device(), instance_->graphics_compute_command_pool(),
                        instance_->graphics_compute_queues()[0], *render_buffers_, semaphore, IMAGE_INDEX);

                delete_buffers();
                create_swapchain_buffers();
        }

        //

        void delete_buffers()
        {
                text_->delete_buffers();
                image_process_.delete_buffers();
                renderer_->delete_buffers();

                image_resolve_.reset();
                object_image_.reset();
                render_buffers_.reset();
        }

        void create_buffers(const VkFormat format, const unsigned width, const unsigned height)
        {
                render_buffers_ = create_render_buffers(
                        RENDER_BUFFER_COUNT, format, DEPTH_FORMATS, width, height,
                        {instance_->graphics_compute_queues()[0].family_index()}, instance_->device(),
                        MINIMUM_SAMPLE_COUNT);

                object_image_.emplace(
                        instance_->device(),
                        std::vector<std::uint32_t>({instance_->graphics_compute_queues()[0].family_index()}),
                        std::vector<VkFormat>({OBJECT_IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers_->width(), render_buffers_->height()),
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_GENERAL,
                        instance_->graphics_compute_command_pool(), instance_->graphics_compute_queues()[0]);

                const auto [window_1, window_2] = window_position_and_size(
                        image_process_.two_windows(), render_buffers_->width(), render_buffers_->height(),
                        frame_size_in_pixels_);

                static_assert(RENDER_BUFFER_COUNT == 1);
                image_resolve_.emplace(
                        instance_->device(), instance_->graphics_compute_command_pool(),
                        instance_->graphics_compute_queues()[0], *render_buffers_, window_1,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);

                renderer_->create_buffers(&render_buffers_->buffers_3d(), &*object_image_, window_1);

                text_->create_buffers(
                        &render_buffers_->buffers_2d(),
                        Region<2, int>({0, 0}, {render_buffers_->width(), render_buffers_->height()}));

                image_process_.create_buffers(
                        &render_buffers_->buffers_2d(), image_resolve_->image(0), *object_image_, window_1, window_2);

                mouse_.set_rectangle(window_1);
                camera_.resize(window_1.width(), window_1.height());
        }

        [[nodiscard]] VkSemaphore render() const
        {
                static_assert(RENDER_BUFFER_COUNT == 1);

                static constexpr int IMAGE_INDEX = 0;
                ASSERT(render_buffers_->image_views().size() == 1);

                VkSemaphore semaphore = renderer_->draw(
                        instance_->graphics_compute_queues()[0], instance_->graphics_compute_queues()[1], IMAGE_INDEX);

                const vulkan::Queue& graphics_queue = instance_->graphics_compute_queues()[0];
                const vulkan::Queue& compute_queue = instance_->compute_queue();

                semaphore = image_process_.draw(*image_resolve_, semaphore, graphics_queue, compute_queue, IMAGE_INDEX);

                if (view_process_.text_active())
                {
                        semaphore = text_->draw(graphics_queue, semaphore, IMAGE_INDEX, frame_rate_.text_data());
                }

                return semaphore;
        }

        //

        void delete_swapchain_buffers()
        {
                swapchain_resolve_.reset();
                delete_buffers();
        }

        void create_swapchain_buffers()
        {
                create_buffers(swapchain_->format(), swapchain_->width(), swapchain_->height());

                swapchain_resolve_.emplace(
                        instance_->device(), instance_->graphics_compute_command_pool(), *render_buffers_, *swapchain_);
        }

        void delete_swapchain()
        {
                instance_->device_wait_idle();

                delete_swapchain_buffers();
                swapchain_.reset();
        }

        void create_swapchain()
        {
                delete_swapchain();

                swapchain_.emplace(
                        instance_->surface(), instance_->device(),
                        std::vector<std::uint32_t>{
                                instance_->graphics_compute_queues()[0].family_index(),
                                instance_->presentation_queue().family_index()},
                        SWAPCHAIN_SURFACE_FORMAT, SWAPCHAIN_PREFERRED_IMAGE_COUNT,
                        view_process_.vertical_sync() ? vulkan::PresentMode::PREFER_SYNC
                                                      : vulkan::PresentMode::PREFER_FAST);

                create_swapchain_buffers();
        }

        [[nodiscard]] bool render_swapchain() const
        {
                const std::optional<std::uint32_t> image_index = vulkan::acquire_next_image(
                        instance_->device(), swapchain_->swapchain(), swapchain_image_semaphore_);

                if (!image_index)
                {
                        return false;
                }

                const vulkan::Queue& queue = instance_->graphics_compute_queues()[0];

                VkSemaphore semaphore = render();

                semaphore = swapchain_resolve_->resolve(queue, swapchain_image_semaphore_, semaphore, *image_index);

                if (!vulkan::queue_present(
                            semaphore, swapchain_->swapchain(), *image_index, instance_->presentation_queue()))
                {
                        return false;
                }

                VULKAN_CHECK(vkQueueWaitIdle(queue));

                return true;
        }

public:
        Impl(const window::WindowID& window, const double window_ppi)
                : frame_size_in_pixels_(frame_size_in_pixels(window_ppi)),
                  frame_rate_(window_ppi),
                  instance_(create_instance(window, device_features())),
                  renderer_(gpu::renderer::create_renderer(
                          instance_.get(),
                          &instance_->graphics_compute_command_pool(),
                          &instance_->graphics_compute_queues()[0],
                          &instance_->transfer_command_pool(),
                          &instance_->transfer_queue(),
                          SAMPLE_RATE_SHADING,
                          SAMPLER_ANISOTROPY)),
                  text_(gpu::text_writer::create_view(
                          instance_.get(),
                          &instance_->graphics_compute_command_pool(),
                          &instance_->graphics_compute_queues()[0],
                          &instance_->transfer_command_pool(),
                          &instance_->transfer_queue(),
                          SAMPLE_RATE_SHADING,
                          frame_rate_.text_size(),
                          DEFAULT_TEXT_COLOR)),
                  image_process_(
                          window_ppi,
                          SAMPLE_RATE_SHADING,
                          instance_.get(),
                          &instance_->graphics_compute_command_pool(),
                          &instance_->graphics_compute_queues()[0],
                          &instance_->transfer_command_pool(),
                          &instance_->transfer_queue(),
                          &instance_->compute_command_pool(),
                          &instance_->compute_queue()),
                  camera_(
                          [this](const auto& info)
                          {
                                  renderer_->send(gpu::renderer::SetCamera(info));
                          }),
                  mouse_(&camera_),
                  clip_plane_(
                          &camera_,
                          [this](const auto& clip_plane)
                          {
                                  renderer_->send(gpu::renderer::SetClipPlane(clip_plane));
                          },
                          [this](const auto& clip_plane_color)
                          {
                                  renderer_->send(gpu::renderer::SetClipPlaneColor(clip_plane_color));
                          }),
                  view_process_(
                          renderer_.get(),
                          text_.get(),
                          &camera_,
                          SWAPCHAIN_INITIAL_VERTICAL_SYNC,
                          [this]
                          {
                                  create_swapchain();
                          })
        {
                create_swapchain();
                command(command::ResetView());
        }

        ~Impl()
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                delete_swapchain();
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        void loop(const std::function<void()>& dispatch_events, std::atomic_bool* const stop)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                FrameClock::time_point last_frame_time = FrameClock::now();
                while (!(*stop))
                {
                        dispatch_events();

                        if (view_process_.text_active())
                        {
                                frame_rate_.calculate();
                        }

                        if (!render_swapchain())
                        {
                                create_swapchain();
                                continue;
                        }

                        if (renderer_->empty())
                        {
                                std::this_thread::sleep_until(last_frame_time + IDLE_MODE_FRAME_DURATION);
                                last_frame_time = FrameClock::now();
                        }
                }
        }

        void send(std::vector<Command>&& commands)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                for (const view::Command& v : commands)
                {
                        std::visit(visitor, v);
                }
        }

        void receive(const std::vector<Info>& infos)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                const auto visitor = [this](const auto& v)
                {
                        info(v);
                };
                for (const Info& v : infos)
                {
                        std::visit(visitor, v);
                }
        }
};
}

std::unique_ptr<View> create_view_impl(
        window::WindowID parent_window,
        double parent_window_ppi,
        std::vector<Command>&& initial_commands)
{
        return std::make_unique<ViewThread<Impl>>(parent_window, parent_window_ppi, std::move(initial_commands));
}
}
