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
#include "render_buffers.h"
#include "swapchain.h"

#include "../com/camera.h"
#include "../com/clip_plane.h"
#include "../com/frame_rate.h"
#include "../com/mouse.h"
#include "../com/view_thread.h"
#include "../com/window.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/com/variant.h>
#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/image/alpha.h>
#include <src/numerical/region.h>
#include <src/vulkan/error.h>
#include <src/vulkan/features.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>
#include <src/window/surface.h>

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
constexpr vulkan::PresentMode SWAPCHAIN_INITIAL_PRESENT_MODE = vulkan::PresentMode::PREFER_FAST;

constexpr VkFormat SAVE_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
constexpr std::array DEPTH_FORMATS = std::to_array<VkFormat>({VK_FORMAT_D32_SFLOAT});

constexpr int MINIMUM_SAMPLE_COUNT = 4;
constexpr bool SAMPLE_RATE_SHADING = true; // supersampling
constexpr bool SAMPLER_ANISOTROPY = true; // anisotropic filtering

constexpr VkFormat OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

constexpr color::Color DEFAULT_TEXT_COLOR{RGB8(255, 255, 255)};

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
        return features;
}

//

std::unique_ptr<vulkan::VulkanInstance> create_instance(const window::WindowID& window)
{
        static_assert(
                2 <= std::tuple_size_v<std::remove_cvref_t<
                        decltype(std::declval<vulkan::VulkanInstance>().graphics_compute_queues())>>);

        const std::vector<std::string> instance_extensions = window::vulkan_create_surface_required_extensions();

        const std::vector<std::string> device_extensions = {};

        const vulkan::DeviceFeatures required_device_features = []()
        {
                vulkan::DeviceFeatures features = device_features();
                vulkan::add_features(&features, ImageProcess::required_device_features());
                vulkan::add_features(&features, gpu::renderer::Renderer::required_device_features());
                vulkan::add_features(&features, gpu::text_writer::View::required_device_features());
                return features;
        }();

        const vulkan::DeviceFeatures optional_device_features = {};

        const std::function<VkSurfaceKHR(VkInstance)> surface_function = [&](const VkInstance instance)
        {
                return window::vulkan_create_surface(window, instance);
        };

        return std::make_unique<vulkan::VulkanInstance>(
                instance_extensions, device_extensions, required_device_features, optional_device_features,
                surface_function);
}

class Impl final
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        const double window_ppi_;
        const int frame_size_in_pixels_ = std::max(1, millimeters_to_pixels(FRAME_SIZE_IN_MILLIMETERS, window_ppi_));

        FrameRate frame_rate_{window_ppi_};
        Camera camera_;
        Mouse mouse_;

        Region<2, int> draw_rectangle_{{Limits<int>::lowest(), Limits<int>::lowest()}, {0, 0}};

        std::optional<Matrix4d> clip_plane_view_matrix_;

        vulkan::PresentMode present_mode_ = SWAPCHAIN_INITIAL_PRESENT_MODE;

        bool text_active_ = true;

        const std::unique_ptr<vulkan::VulkanInstance> instance_;

        std::unique_ptr<gpu::renderer::Renderer> renderer_;
        std::unique_ptr<gpu::text_writer::View> text_;
        std::unique_ptr<ImageProcess> image_process_;

        const vulkan::handle::Semaphore swapchain_image_semaphore_{instance_->device()};
        std::unique_ptr<vulkan::Swapchain> swapchain_;
        std::unique_ptr<Swapchain> swapchain_resolve_;

        std::unique_ptr<RenderBuffers> render_buffers_;
        std::unique_ptr<ImageResolve> image_resolve_;
        std::unique_ptr<vulkan::ImageWithMemory> object_image_;

        void clip_plane_show(const double position)
        {
                clip_plane_view_matrix_ = camera_.renderer_info().main_view_matrix;
                clip_plane_position(position);
        }

        void clip_plane_position(const double position)
        {
                if (!clip_plane_view_matrix_)
                {
                        error("Clip plane is not set");
                }
                if (!(position >= 0.0 && position <= 1.0))
                {
                        error("Error clip plane position " + to_string(position));
                }

                renderer_->set_clip_plane(create_clip_plane(*clip_plane_view_matrix_, position));
        }

        void clip_plane_hide()
        {
                clip_plane_view_matrix_.reset();
                renderer_->set_clip_plane(std::nullopt);
        }

        void mouse_move(const int x, const int y)
        {
                mouse_.move(x, y);

                bool changed = false;

                const MouseButtonInfo& right = mouse_.info(MouseButton::RIGHT);
                if (right.pressed && draw_rectangle_.is_inside(right.pressed_x, right.pressed_y)
                    && (right.delta_x != 0 || right.delta_y != 0))
                {
                        camera_.rotate(-right.delta_x, -right.delta_y);
                        changed = true;
                }

                const MouseButtonInfo& left = mouse_.info(MouseButton::LEFT);
                if (left.pressed && draw_rectangle_.is_inside(left.pressed_x, left.pressed_y)
                    && (left.delta_x != 0 || left.delta_y != 0))
                {
                        camera_.move(Vector2d(-left.delta_x, left.delta_y));
                        changed = true;
                }

                if (changed)
                {
                        renderer_->set_camera(camera_.renderer_info());
                }
        }

        //

        void reset_view_handler()
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                camera_.reset(Vector3d(1, 0, 0), Vector3d(0, 1, 0), 1, Vector2d(0, 0));

                renderer_->set_camera(camera_.renderer_info());
        }

        void set_vertical_sync_swapchain(const bool v)
        {
                if (v && present_mode_ != vulkan::PresentMode::PREFER_SYNC)
                {
                        present_mode_ = vulkan::PresentMode::PREFER_SYNC;
                        create_swapchain();
                        return;
                }
                if (!v && present_mode_ != vulkan::PresentMode::PREFER_FAST)
                {
                        present_mode_ = vulkan::PresentMode::PREFER_FAST;
                        create_swapchain();
                        return;
                }
        }

        //

        void command(const command::UpdateMeshObject& v)
        {
                if (auto ptr = v.object.lock(); ptr)
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::UpdateVolumeObject& v)
        {
                if (auto ptr = v.object.lock(); ptr)
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::DeleteObject& v)
        {
                renderer_->object_delete(v.id);
        }

        void command(const command::ShowObject& v)
        {
                renderer_->object_show(v.id, v.show);
        }

        void command(const command::DeleteAllObjects&)
        {
                renderer_->object_delete_all();
                reset_view_handler();
        }

        void command(const command::ResetView&)
        {
                reset_view_handler();
        }

        void command(const command::SetLightingColor& v)
        {
                renderer_->set_lighting_color(v.value);
        }

        void command(const command::SetBackgroundColor& v)
        {
                renderer_->set_background_color(v.value);
                const bool background_is_dark = v.value.luminance() <= 0.5;
                if (background_is_dark)
                {
                        static const color::Color white(1);
                        text_->set_color(white);
                }
                else
                {
                        static const color::Color black(0);
                        text_->set_color(black);
                }
        }

        void command(const command::SetWireframeColor& v)
        {
                renderer_->set_wireframe_color(v.value);
        }

        void command(const command::SetClipPlaneColor& v)
        {
                renderer_->set_clip_plane_color(v.value);
        }

        void command(const command::SetNormalLength& v)
        {
                renderer_->set_normal_length(v.value);
        }

        void command(const command::SetNormalColorPositive& v)
        {
                renderer_->set_normal_color_positive(v.value);
        }

        void command(const command::SetNormalColorNegative& v)
        {
                renderer_->set_normal_color_negative(v.value);
        }

        void command(const command::ShowSmooth& v)
        {
                renderer_->set_show_smooth(v.show);
        }

        void command(const command::ShowWireframe& v)
        {
                renderer_->set_show_wireframe(v.show);
        }

        void command(const command::ShowShadow& v)
        {
                renderer_->set_show_shadow(v.show);
        }

        void command(const command::ShowFog& v)
        {
                renderer_->set_show_fog(v.show);
        }

        void command(const command::ShowMaterials& v)
        {
                renderer_->set_show_materials(v.show);
        }

        void command(const command::ShowFps& d)
        {
                text_active_ = d.show;
        }

        void command(const command::SetVerticalSync& v)
        {
                set_vertical_sync_swapchain(v.enabled);
        }

        void command(const command::SetShadowZoom& v)
        {
                renderer_->set_shadow_zoom(v.value);
        }

        void command(const command::ClipPlaneShow& v)
        {
                clip_plane_show(v.position);
        }

        void command(const command::ClipPlanePosition& v)
        {
                clip_plane_position(v.position);
        }

        void command(const command::ClipPlaneHide&)
        {
                clip_plane_hide();
        }

        void command(const command::ShowNormals& v)
        {
                renderer_->set_show_normals(v.show);
        }

        void command(const command::MousePress& v)
        {
                mouse_.press(v.x, v.y, v.button);
        }

        void command(const command::MouseRelease& v)
        {
                mouse_.release(v.x, v.y, v.button);
        }

        void command(const command::MouseMove& v)
        {
                mouse_move(v.x, v.y);
        }

        void command(const command::MouseWheel& v)
        {
                camera_.scale(v.x - draw_rectangle_.x0(), v.y - draw_rectangle_.y0(), v.delta);
                renderer_->set_camera(camera_.renderer_info());
        }

        void command(const command::WindowResize&)
        {
        }

        //

        void command(const ViewCommand& view_command)
        {
                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                std::visit(visitor, view_command);
        }

        void command(const ImageCommand& image_command)
        {
                const bool two_windows = image_process_->two_windows();
                image_process_->command(image_command);
                if (two_windows != image_process_->two_windows())
                {
                        create_swapchain();
                }
        }

        //

        void info(info::Camera* const camera_info) const
        {
                *camera_info = camera_.view_info();
        }

        void info(info::Image* const image_info)
        {
                static_assert(RENDER_BUFFER_COUNT == 1);
                constexpr int INDEX = 0;

                instance_->device_wait_idle();

                const int width = swapchain_->width();
                const int height = swapchain_->height();

                delete_swapchain_buffers();
                create_buffers(SAVE_FORMAT, width, height);

                {
                        VkSemaphore semaphore = render();

                        const vulkan::Queue& queue = instance_->graphics_compute_queues()[0];

                        const ImageResolve image(
                                instance_->device(), instance_->graphics_compute_command_pool(), queue,
                                *render_buffers_, Region<2, int>({0, 0}, {width, height}), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                        image.resolve(queue, semaphore, INDEX);
                        VULKAN_CHECK(vkQueueWaitIdle(queue));

                        image.image(INDEX).read_pixels(
                                instance_->graphics_compute_command_pool(), queue, VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_GENERAL, &image_info->image.color_format, &image_info->image.pixels);
                }

                image_info->image.size[0] = width;
                image_info->image.size[1] = height;

                ASSERT(4 == image::format_component_count(image_info->image.color_format));
                image_info->image = image::delete_alpha(image_info->image);

                delete_buffers();
                create_swapchain_buffers();
        }

        //

        void delete_buffers()
        {
                text_->delete_buffers();
                image_process_->delete_buffers();
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

                object_image_ = std::make_unique<vulkan::ImageWithMemory>(
                        instance_->device(),
                        std::vector<std::uint32_t>({instance_->graphics_compute_queues()[0].family_index()}),
                        std::vector<VkFormat>({OBJECT_IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers_->width(), render_buffers_->height()),
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_GENERAL,
                        instance_->graphics_compute_command_pool(), instance_->graphics_compute_queues()[0]);

                const auto [window_1, window_2] = window_position_and_size(
                        image_process_->two_windows(), render_buffers_->width(), render_buffers_->height(),
                        frame_size_in_pixels_);

                draw_rectangle_ = window_1;

                static_assert(RENDER_BUFFER_COUNT == 1);
                image_resolve_ = std::make_unique<ImageResolve>(
                        instance_->device(), instance_->graphics_compute_command_pool(),
                        instance_->graphics_compute_queues()[0], *render_buffers_, window_1,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);

                renderer_->create_buffers(&render_buffers_->buffers_3d(), object_image_.get(), window_1);

                text_->create_buffers(
                        &render_buffers_->buffers_2d(),
                        Region<2, int>({0, 0}, {render_buffers_->width(), render_buffers_->height()}));

                image_process_->create_buffers(
                        &render_buffers_->buffers_2d(), image_resolve_->image(0), *object_image_, window_1, window_2);

                camera_.resize(window_1.width(), window_1.height());
                renderer_->set_camera(camera_.renderer_info());
        }

        [[nodiscard]] VkSemaphore render() const
        {
                static_assert(RENDER_BUFFER_COUNT == 1);
                ASSERT(render_buffers_->image_views().size() == 1);

                constexpr int INDEX = 0;

                VkSemaphore semaphore = renderer_->draw(
                        instance_->graphics_compute_queues()[0], instance_->graphics_compute_queues()[1], INDEX);

                const vulkan::Queue& graphics_queue = instance_->graphics_compute_queues()[0];
                const vulkan::Queue& compute_queue = instance_->compute_queue();

                semaphore = image_process_->draw(*image_resolve_, semaphore, graphics_queue, compute_queue, INDEX);

                if (text_active_)
                {
                        semaphore = text_->draw(graphics_queue, semaphore, INDEX, frame_rate_.text_data());
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

                swapchain_resolve_ = std::make_unique<Swapchain>(
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

                swapchain_ = std::make_unique<vulkan::Swapchain>(
                        instance_->surface(), instance_->device(),
                        std::vector<std::uint32_t>{
                                instance_->graphics_compute_queues()[0].family_index(),
                                instance_->presentation_queue().family_index()},
                        SWAPCHAIN_SURFACE_FORMAT, SWAPCHAIN_PREFERRED_IMAGE_COUNT, present_mode_);

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
                : window_ppi_(window_ppi), instance_(create_instance(window))
        {
                if (!(window_ppi_ > 0))
                {
                        error("Window PPI " + to_string(window_ppi_) + "is not positive");
                }

                renderer_ = gpu::renderer::create_renderer(
                        instance_.get(), &instance_->graphics_compute_command_pool(),
                        &instance_->graphics_compute_queues()[0], &instance_->transfer_command_pool(),
                        &instance_->transfer_queue(), SAMPLE_RATE_SHADING, SAMPLER_ANISOTROPY);

                text_ = gpu::text_writer::create_view(
                        instance_.get(), &instance_->graphics_compute_command_pool(),
                        &instance_->graphics_compute_queues()[0], &instance_->transfer_command_pool(),
                        &instance_->transfer_queue(), SAMPLE_RATE_SHADING, frame_rate_.text_size(), DEFAULT_TEXT_COLOR);

                image_process_ = std::make_unique<ImageProcess>(
                        window_ppi, SAMPLE_RATE_SHADING, instance_.get(), &instance_->graphics_compute_command_pool(),
                        &instance_->graphics_compute_queues()[0], &instance_->transfer_command_pool(),
                        &instance_->transfer_queue(), &instance_->compute_command_pool(), &instance_->compute_queue());

                create_swapchain();

                reset_view_handler();
                clip_plane_hide();
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

                        if (text_active_)
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
