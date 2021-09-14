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
#include <src/gpu/convex_hull/view.h>
#include <src/gpu/dft/view.h>
#include <src/gpu/optical_flow/view.h>
#include <src/gpu/pencil_sketch/view.h>
#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/image/alpha.h>
#include <src/numerical/region.h>
#include <src/vulkan/features.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>
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
                vulkan::add_features(&features, gpu::convex_hull::View::required_device_features());
                vulkan::add_features(&features, gpu::dft::View::required_device_features());
                vulkan::add_features(&features, gpu::optical_flow::View::required_device_features());
                vulkan::add_features(&features, gpu::pencil_sketch::View::required_device_features());
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

        Region<2, int> draw_rectangle_{limits<int>::lowest(), limits<int>::lowest(), 0, 0};

        std::optional<Matrix4d> clip_plane_view_matrix_;

        vulkan::PresentMode present_mode_ = SWAPCHAIN_INITIAL_PRESENT_MODE;

        bool text_active_ = true;
        bool convex_hull_active_ = false;
        bool pencil_sketch_active_ = false;
        bool dft_active_ = false;
        bool optical_flow_active_ = false;

        const std::unique_ptr<vulkan::VulkanInstance> instance_;

        std::unique_ptr<gpu::renderer::Renderer> renderer_;
        std::unique_ptr<gpu::text_writer::View> text_;
        std::unique_ptr<gpu::convex_hull::View> convex_hull_;
        std::unique_ptr<gpu::pencil_sketch::View> pencil_sketch_;
        std::unique_ptr<gpu::dft::View> dft_;
        std::unique_ptr<gpu::optical_flow::View> optical_flow_;

        const vulkan::Semaphore swapchain_image_semaphore_{instance_->device()};
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

        void command(const command::UpdateMeshObject& d)
        {
                if (auto ptr = d.object.lock(); ptr)
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::UpdateVolumeObject& d)
        {
                if (auto ptr = d.object.lock(); ptr)
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::DeleteObject& d)
        {
                renderer_->object_delete(d.id);
        }

        void command(const command::ShowObject& d)
        {
                renderer_->object_show(d.id, d.show);
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

        void command(const command::SetBackgroundColor& d)
        {
                renderer_->set_background_color(d.value);
                const bool background_is_dark = d.value.luminance() <= 0.5;
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

        void command(const command::SetWireframeColor& d)
        {
                renderer_->set_wireframe_color(d.value);
        }

        void command(const command::SetClipPlaneColor& d)
        {
                renderer_->set_clip_plane_color(d.value);
        }

        void command(const command::SetNormalLength& d)
        {
                renderer_->set_normal_length(d.value);
        }

        void command(const command::SetNormalColorPositive& d)
        {
                renderer_->set_normal_color_positive(d.value);
        }

        void command(const command::SetNormalColorNegative& d)
        {
                renderer_->set_normal_color_negative(d.value);
        }

        void command(const command::ShowSmooth& d)
        {
                renderer_->set_show_smooth(d.show);
        }

        void command(const command::ShowWireframe& d)
        {
                renderer_->set_show_wireframe(d.show);
        }

        void command(const command::ShowShadow& d)
        {
                renderer_->set_show_shadow(d.show);
        }

        void command(const command::ShowFog& d)
        {
                renderer_->set_show_fog(d.show);
        }

        void command(const command::ShowMaterials& d)
        {
                renderer_->set_show_materials(d.show);
        }

        void command(const command::ShowFps& d)
        {
                text_active_ = d.show;
        }

        void command(const command::ShowPencilSketch& d)
        {
                pencil_sketch_active_ = d.show;
        }

        void command(const command::ShowDft& d)
        {
                if (dft_active_ != d.show)
                {
                        dft_active_ = d.show;
                        create_swapchain();
                }
        }

        void command(const command::SetDftBrightness& d)
        {
                dft_->set_brightness(d.value);
        }

        void command(const command::SetDftBackgroundColor& d)
        {
                dft_->set_background_color(d.value);
        }

        void command(const command::SetDftColor& d)
        {
                dft_->set_color(d.value);
        }

        void command(const command::ShowConvexHull2D& d)
        {
                convex_hull_active_ = d.show;
                if (convex_hull_active_)
                {
                        convex_hull_->reset_timer();
                }
        }

        void command(const command::ShowOpticalFlow& d)
        {
                optical_flow_active_ = d.show;
        }

        void command(const command::SetVerticalSync& d)
        {
                set_vertical_sync_swapchain(d.enabled);
        }

        void command(const command::SetShadowZoom& d)
        {
                renderer_->set_shadow_zoom(d.value);
        }

        void command(const command::ClipPlaneShow& d)
        {
                clip_plane_show(d.position);
        }

        void command(const command::ClipPlanePosition& d)
        {
                clip_plane_position(d.position);
        }

        void command(const command::ClipPlaneHide&)
        {
                clip_plane_hide();
        }

        void command(const command::ShowNormals& d)
        {
                renderer_->set_show_normals(d.show);
        }

        void command(const command::MousePress& d)
        {
                mouse_.press(d.x, d.y, d.button);
        }

        void command(const command::MouseRelease& d)
        {
                mouse_.release(d.x, d.y, d.button);
        }

        void command(const command::MouseMove& d)
        {
                mouse_move(d.x, d.y);
        }

        void command(const command::MouseWheel& d)
        {
                camera_.scale(d.x - draw_rectangle_.x0(), d.y - draw_rectangle_.y0(), d.delta);
                renderer_->set_camera(camera_.renderer_info());
        }

        void command(const command::WindowResize&)
        {
        }

        //

        void info(info::Camera* const d) const
        {
                *d = camera_.view_info();
        }

        void info(info::Image* const d)
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
                                *render_buffers_, Region<2, int>(0, 0, width, height), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                        image.resolve(queue, semaphore, INDEX);
                        vulkan::queue_wait_idle(queue);

                        image.image(INDEX).read_pixels(
                                instance_->graphics_compute_command_pool(), queue, VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_GENERAL, &d->image.color_format, &d->image.pixels);
                }

                d->image.size[0] = width;
                d->image.size[1] = height;

                ASSERT(4 == image::format_component_count(d->image.color_format));
                d->image = image::delete_alpha(d->image);

                delete_buffers();
                create_swapchain_buffers();
        }

        //

        void delete_buffers()
        {
                text_->delete_buffers();
                convex_hull_->delete_buffers();
                pencil_sketch_->delete_buffers();
                dft_->delete_buffers();
                optical_flow_->delete_buffers();
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
                        std::vector<uint32_t>({instance_->graphics_compute_queues()[0].family_index()}),
                        std::vector<VkFormat>({OBJECT_IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(render_buffers_->width(), render_buffers_->height()),
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_GENERAL,
                        instance_->graphics_compute_command_pool(), instance_->graphics_compute_queues()[0]);

                const auto [w_1, w_2] = window_position_and_size(
                        dft_active_, render_buffers_->width(), render_buffers_->height(), frame_size_in_pixels_);

                draw_rectangle_ = w_1;

                static_assert(RENDER_BUFFER_COUNT == 1);
                image_resolve_ = std::make_unique<ImageResolve>(
                        instance_->device(), instance_->graphics_compute_command_pool(),
                        instance_->graphics_compute_queues()[0], *render_buffers_, draw_rectangle_,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);

                renderer_->create_buffers(&render_buffers_->buffers_3d(), object_image_.get(), draw_rectangle_);

                text_->create_buffers(
                        &render_buffers_->buffers_2d(),
                        Region<2, int>(0, 0, render_buffers_->width(), render_buffers_->height()));

                convex_hull_->create_buffers(&render_buffers_->buffers_2d(), *object_image_, draw_rectangle_);

                pencil_sketch_->create_buffers(
                        &render_buffers_->buffers_2d(), image_resolve_->image(0), *object_image_, draw_rectangle_);

                optical_flow_->create_buffers(
                        &render_buffers_->buffers_2d(), image_resolve_->image(0), window_ppi_, draw_rectangle_);

                if (w_2)
                {
                        dft_->create_buffers(
                                &render_buffers_->buffers_2d(), image_resolve_->image(0), draw_rectangle_, *w_2);
                }

                camera_.resize(draw_rectangle_.width(), draw_rectangle_.height());
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

                if (pencil_sketch_active_)
                {
                        semaphore = image_resolve_->resolve_semaphore(graphics_queue, semaphore, INDEX);
                        semaphore = pencil_sketch_->draw(graphics_queue, semaphore, INDEX);
                }

                if (dft_active_ || optical_flow_active_)
                {
                        semaphore = image_resolve_->resolve_semaphore(graphics_queue, semaphore, INDEX);
                }

                if (dft_active_)
                {
                        semaphore = dft_->draw(graphics_queue, semaphore, INDEX);
                }

                if (optical_flow_active_)
                {
                        semaphore = optical_flow_->draw(graphics_queue, compute_queue, semaphore, INDEX);
                }

                if (convex_hull_active_)
                {
                        semaphore = convex_hull_->draw(graphics_queue, semaphore, INDEX);
                }

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
                        std::vector<uint32_t>{
                                instance_->graphics_compute_queues()[0].family_index(),
                                instance_->presentation_queue().family_index()},
                        SWAPCHAIN_SURFACE_FORMAT, SWAPCHAIN_PREFERRED_IMAGE_COUNT, present_mode_);

                create_swapchain_buffers();
        }

        [[nodiscard]] bool render_swapchain() const
        {
                const std::optional<uint32_t> image_index = vulkan::acquire_next_image(
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

                vulkan::queue_wait_idle(queue);

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

                const vulkan::Queue& graphics_compute_queue = instance_->graphics_compute_queues()[0];
                const vulkan::CommandPool& graphics_compute_command_pool = instance_->graphics_compute_command_pool();
                const vulkan::Queue& compute_queue = instance_->compute_queue();
                const vulkan::CommandPool& compute_command_pool = instance_->compute_command_pool();
                const vulkan::Queue& transfer_queue = instance_->transfer_queue();
                const vulkan::CommandPool& transfer_command_pool = instance_->transfer_command_pool();

                renderer_ = gpu::renderer::create_renderer(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING, SAMPLER_ANISOTROPY);

                text_ = gpu::text_writer::create_view(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING, frame_rate_.text_size(), DEFAULT_TEXT_COLOR);

                convex_hull_ = gpu::convex_hull::create_view(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, SAMPLE_RATE_SHADING);

                pencil_sketch_ = gpu::pencil_sketch::create_view(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING);

                dft_ = gpu::dft::create_view(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING);

                optical_flow_ = gpu::optical_flow::create_view(
                        *instance_, graphics_compute_command_pool, graphics_compute_queue, compute_command_pool,
                        compute_queue, transfer_command_pool, transfer_queue, SAMPLE_RATE_SHADING);

                //

                create_swapchain();

                //

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
