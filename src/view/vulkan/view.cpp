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

#include "image.h"
#include "render_buffer.h"
#include "swapchain.h"

#include "../com/camera.h"
#include "../com/frame_rate.h"
#include "../com/view_thread.h"
#include "../com/window.h"

#include <src/com/alg.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/merge.h>
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

constexpr int PREFERRED_IMAGE_COUNT = 2; // 2 - double buffering, 3 - triple buffering
constexpr VkSurfaceFormatKHR SURFACE_FORMAT = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

constexpr int MINIMUM_SAMPLE_COUNT = 4;
constexpr bool SAMPLE_RATE_SHADING = true; // supersampling
constexpr bool SAMPLER_ANISOTROPY = true; // anisotropic filtering

constexpr VkFormat OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

constexpr vulkan::PresentMode DEFAULT_PRESENT_MODE = vulkan::PresentMode::PreferFast;
constexpr RGB8 DEFAULT_TEXT_COLOR = RGB8(255, 255, 255);

//

double checked_window_ppi(double window_ppi)
{
        if (!(window_ppi > 0))
        {
                error("Window PPI " + to_string(window_ppi) + "is not positive");
        }
        return window_ppi;
}

std::vector<vulkan::PhysicalDeviceFeatures> view_required_device_features()
{
        std::vector<vulkan::PhysicalDeviceFeatures> features;
        if (MINIMUM_SAMPLE_COUNT > 1 && SAMPLE_RATE_SHADING)
        {
                features.push_back(vulkan::PhysicalDeviceFeatures::sampleRateShading);
        }
        if (SAMPLER_ANISOTROPY)
        {
                features.push_back(vulkan::PhysicalDeviceFeatures::samplerAnisotropy);
        }
        return features;
}

std::unique_ptr<vulkan::VulkanInstance> create_instance(const window::WindowID& window)
{
        const std::vector<std::string> instance_extensions =
                unique_elements(window::vulkan_create_surface_required_extensions());

        const std::vector<std::string> device_extensions = {};

        const std::vector<vulkan::PhysicalDeviceFeatures> required_device_features =
                unique_elements(merge<std::vector<vulkan::PhysicalDeviceFeatures>>(
                        gpu::convex_hull::View::required_device_features(), gpu::dft::View::required_device_features(),
                        gpu::optical_flow::View::required_device_features(),
                        gpu::pencil_sketch::View::required_device_features(),
                        gpu::renderer::Renderer::required_device_features(),
                        gpu::text_writer::View::required_device_features(), view_required_device_features()));

        const std::vector<vulkan::PhysicalDeviceFeatures> optional_device_features = {};

        const std::function<VkSurfaceKHR(VkInstance)> surface_function = [&](VkInstance instance)
        {
                return window::vulkan_create_surface(window, instance);
        };

        std::unique_ptr<vulkan::VulkanInstance> instance = std::make_unique<vulkan::VulkanInstance>(
                instance_extensions, device_extensions, required_device_features, optional_device_features,
                surface_function);

        ASSERT(instance->graphics_compute_command_pool().family_index()
               == instance->graphics_compute_queues()[0].family_index());
        ASSERT(instance->compute_command_pool().family_index() == instance->compute_queue().family_index());
        ASSERT(instance->transfer_command_pool().family_index() == instance->transfer_queue().family_index());

        return instance;
}

class Impl final
{
        static_assert(
                2 <= std::tuple_size_v<std::remove_cvref_t<
                        decltype(std::declval<vulkan::VulkanInstance>().graphics_compute_queues())>>);

        static constexpr unsigned RENDER_BUFFER_COUNT = 1;

        const std::thread::id m_thread_id = std::this_thread::get_id();
        const double m_window_ppi;
        const int m_frame_size_in_pixels = std::max(1, millimeters_to_pixels(FRAME_SIZE_IN_MILLIMETERS, m_window_ppi));

        FrameRate m_frame_rate{m_window_ppi};
        Camera m_camera;

        Region<2, int> m_draw_rectangle{limits<int>::lowest(), limits<int>::lowest(), 0, 0};

        std::optional<mat4d> m_clip_plane_view_matrix;

        vulkan::PresentMode m_present_mode = DEFAULT_PRESENT_MODE;

        bool m_text_active = true;
        bool m_convex_hull_active = false;
        bool m_pencil_sketch_active = false;
        bool m_dft_active = false;
        bool m_optical_flow_active = false;

        const std::unique_ptr<vulkan::VulkanInstance> m_instance;

        std::unique_ptr<gpu::renderer::Renderer> m_renderer;
        std::unique_ptr<gpu::text_writer::View> m_text;
        std::unique_ptr<gpu::convex_hull::View> m_convex_hull;
        std::unique_ptr<gpu::pencil_sketch::View> m_pencil_sketch;
        std::unique_ptr<gpu::dft::View> m_dft;
        std::unique_ptr<gpu::optical_flow::View> m_optical_flow;

        const vulkan::Semaphore m_swapchain_image_semaphore{m_instance->device()};
        std::unique_ptr<vulkan::Swapchain> m_swapchain;
        std::unique_ptr<Swapchain> m_swapchain_resolve;

        std::unique_ptr<RenderBuffers> m_render_buffers;
        std::unique_ptr<Image> m_image_resolve;
        std::unique_ptr<vulkan::ImageWithMemory> m_object_image;

        struct PressedMouseButton
        {
                bool pressed = false;
                int pressed_x;
                int pressed_y;
                int delta_x;
                int delta_y;
        };
        std::unordered_map<command::MouseButton, PressedMouseButton> m_mouse;
        int m_mouse_x = std::numeric_limits<int>::lowest();
        int m_mouse_y = std::numeric_limits<int>::lowest();

        const PressedMouseButton& pressed_mouse_button(command::MouseButton button) const
        {
                auto iter = m_mouse.find(button);
                if (iter != m_mouse.cend())
                {
                        return iter->second;
                }

                thread_local const PressedMouseButton m{};
                return m;
        }

        void clip_plane_show(double position)
        {
                m_clip_plane_view_matrix = m_camera.renderer_info().main_view_matrix;
                clip_plane_position(position);
        }

        void clip_plane_position(double position)
        {
                if (!m_clip_plane_view_matrix)
                {
                        error("Clip plane is not set");
                }

                ASSERT(position >= 0.0 && position <= 1.0);

                // Уравнение плоскости
                // -z = 0 или (0, 0, -1, 0).
                // Уравнение плоскости для исходных координат
                // (0, 0, -1, 0) * view matrix.
                vec4d plane = -m_clip_plane_view_matrix->row(2);

                vec3d n(plane[0], plane[1], plane[2]);
                double d = n.norm_1();

                // Уравнение плоскости со смещением
                // -z = d * (1 - 2 * position) или (0, 0, -1, d * (2 * position - 1)).
                plane[3] += d * (2 * position - 1);

                plane /= n.norm();

                m_renderer->set_clip_plane(plane);
        }

        void clip_plane_hide()
        {
                m_clip_plane_view_matrix.reset();
                m_renderer->set_clip_plane(std::nullopt);
        }

        void mouse_move(int x, int y)
        {
                for (auto& [button, m] : m_mouse)
                {
                        if (m.pressed)
                        {
                                m.delta_x = x - m_mouse_x;
                                m.delta_y = y - m_mouse_y;
                        }
                }
                m_mouse_x = x;
                m_mouse_y = y;

                //

                bool changed = false;

                const PressedMouseButton& right = pressed_mouse_button(command::MouseButton::Right);
                if (right.pressed && m_draw_rectangle.is_inside(right.pressed_x, right.pressed_y)
                    && (right.delta_x != 0 || right.delta_y != 0))
                {
                        m_camera.rotate(-right.delta_x, -right.delta_y);
                        changed = true;
                }

                const PressedMouseButton& left = pressed_mouse_button(command::MouseButton::Left);
                if (left.pressed && m_draw_rectangle.is_inside(left.pressed_x, left.pressed_y)
                    && (left.delta_x != 0 || left.delta_y != 0))
                {
                        m_camera.move(vec2d(-left.delta_x, left.delta_y));
                        changed = true;
                }

                if (changed)
                {
                        m_renderer->set_camera(m_camera.renderer_info());
                }
        }

        //

        void reset_view_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.reset(vec3d(1, 0, 0), vec3d(0, 1, 0), 1, vec2d(0, 0));

                m_renderer->set_camera(m_camera.renderer_info());
        }

        void set_vertical_sync_swapchain(bool v)
        {
                if (v && m_present_mode != vulkan::PresentMode::PreferSync)
                {
                        m_present_mode = vulkan::PresentMode::PreferSync;
                        create_swapchain();
                        return;
                }
                if (!v && m_present_mode != vulkan::PresentMode::PreferFast)
                {
                        m_present_mode = vulkan::PresentMode::PreferFast;
                        create_swapchain();
                        return;
                }
        }

        //

        void command(const command::UpdateMeshObject& d)
        {
                if (auto ptr = d.object.lock(); ptr)
                {
                        m_renderer->object_update(*ptr);
                }
        }

        void command(const command::UpdateVolumeObject& d)
        {
                if (auto ptr = d.object.lock(); ptr)
                {
                        m_renderer->object_update(*ptr);
                }
        }

        void command(const command::DeleteObject& d)
        {
                m_renderer->object_delete(d.id);
        }

        void command(const command::ShowObject& d)
        {
                m_renderer->object_show(d.id, d.show);
        }

        void command(const command::DeleteAllObjects&)
        {
                m_renderer->object_delete_all();
                reset_view_handler();
        }

        void command(const command::ResetView&)
        {
                reset_view_handler();
        }

        void command(const command::SetLightingColor& v)
        {
                m_renderer->set_lighting_color(v.value);
        }

        void command(const command::SetBackgroundColor& d)
        {
                m_renderer->set_background_color(d.value);
                const bool background_is_dark = d.value.luminance() <= 0.5;
                if (background_is_dark)
                {
                        static const color::Color white(1);
                        m_text->set_color(white);
                }
                else
                {
                        static const color::Color black(0);
                        m_text->set_color(black);
                }
        }

        void command(const command::SetWireframeColor& d)
        {
                m_renderer->set_wireframe_color(d.value);
        }

        void command(const command::SetClipPlaneColor& d)
        {
                m_renderer->set_clip_plane_color(d.value);
        }

        void command(const command::SetNormalLength& d)
        {
                m_renderer->set_normal_length(d.value);
        }

        void command(const command::SetNormalColorPositive& d)
        {
                m_renderer->set_normal_color_positive(d.value);
        }

        void command(const command::SetNormalColorNegative& d)
        {
                m_renderer->set_normal_color_negative(d.value);
        }

        void command(const command::ShowSmooth& d)
        {
                m_renderer->set_show_smooth(d.show);
        }

        void command(const command::ShowWireframe& d)
        {
                m_renderer->set_show_wireframe(d.show);
        }

        void command(const command::ShowShadow& d)
        {
                m_renderer->set_show_shadow(d.show);
        }

        void command(const command::ShowFog& d)
        {
                m_renderer->set_show_fog(d.show);
        }

        void command(const command::ShowMaterials& d)
        {
                m_renderer->set_show_materials(d.show);
        }

        void command(const command::ShowFps& d)
        {
                m_text_active = d.show;
        }

        void command(const command::ShowPencilSketch& d)
        {
                m_pencil_sketch_active = d.show;
        }

        void command(const command::ShowDft& d)
        {
                if (m_dft_active != d.show)
                {
                        m_dft_active = d.show;
                        create_swapchain();
                }
        }

        void command(const command::SetDftBrightness& d)
        {
                m_dft->set_brightness(d.value);
        }

        void command(const command::SetDftBackgroundColor& d)
        {
                m_dft->set_background_color(d.value);
        }

        void command(const command::SetDftColor& d)
        {
                m_dft->set_color(d.value);
        }

        void command(const command::ShowConvexHull2D& d)
        {
                m_convex_hull_active = d.show;
                if (m_convex_hull_active)
                {
                        m_convex_hull->reset_timer();
                }
        }

        void command(const command::ShowOpticalFlow& d)
        {
                m_optical_flow_active = d.show;
        }

        void command(const command::SetVerticalSync& d)
        {
                set_vertical_sync_swapchain(d.enabled);
        }

        void command(const command::SetShadowZoom& d)
        {
                m_renderer->set_shadow_zoom(d.value);
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
                m_renderer->set_show_normals(d.show);
        }

        void command(const command::MousePress& d)
        {
                m_mouse_x = d.x;
                m_mouse_y = d.y;
                PressedMouseButton& m = m_mouse[d.button];
                m.pressed = true;
                m.pressed_x = m_mouse_x;
                m.pressed_y = m_mouse_y;
                m.delta_x = 0;
                m.delta_y = 0;
        }

        void command(const command::MouseRelease& d)
        {
                m_mouse[d.button].pressed = false;
                m_mouse_x = d.x;
                m_mouse_y = d.y;
        }

        void command(const command::MouseMove& d)
        {
                mouse_move(d.x, d.y);
        }

        void command(const command::MouseWheel& d)
        {
                m_camera.scale(d.x - m_draw_rectangle.x0(), d.y - m_draw_rectangle.y0(), d.delta);
                m_renderer->set_camera(m_camera.renderer_info());
        }

        void command(const command::WindowResize&)
        {
        }

        //

        void info(info::Camera* const d) const
        {
                *d = m_camera.view_info();
        }

        void info(info::Image* const d) const
        {
                static_assert(RENDER_BUFFER_COUNT == 1);
                ASSERT(m_render_buffers->image_views().size() == 1);

                constexpr int INDEX = 0;

                const vulkan::Queue& queue = m_instance->graphics_compute_queues()[0];

                const int width = m_render_buffers->width();
                const int height = m_render_buffers->height();

                const Image image(
                        m_instance->device(), m_instance->graphics_compute_command_pool(), queue, *m_render_buffers,
                        Region<2, int>(0, 0, width, height), VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                image.resolve(queue, INDEX);

                vulkan::queue_wait_idle(queue);

                d->image.size[0] = width;
                d->image.size[1] = height;

                image.image(INDEX).read_pixels(
                        m_instance->graphics_compute_command_pool(), queue, VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL, &d->image.color_format, &d->image.pixels);

                ASSERT(4 == image::format_component_count(d->image.color_format));
                d->image = image::delete_alpha(d->image);
        }

        //

        void delete_buffers()
        {
                m_instance->device_wait_idle();

                m_text->delete_buffers();
                m_convex_hull->delete_buffers();
                m_pencil_sketch->delete_buffers();
                m_dft->delete_buffers();
                m_optical_flow->delete_buffers();
                m_renderer->delete_buffers();

                m_image_resolve.reset();
                m_object_image.reset();
                m_render_buffers.reset();
        }

        void create_buffers(VkFormat format, unsigned width, unsigned height)
        {
                delete_buffers();

                m_render_buffers = create_render_buffers(
                        RENDER_BUFFER_COUNT, format, width, height,
                        {m_instance->graphics_compute_queues()[0].family_index()}, m_instance->device(),
                        MINIMUM_SAMPLE_COUNT);

                m_object_image = std::make_unique<vulkan::ImageWithMemory>(
                        m_instance->device(), m_instance->graphics_compute_command_pool(),
                        m_instance->graphics_compute_queues()[0],
                        std::vector<uint32_t>({m_instance->graphics_compute_queues()[0].family_index()}),
                        std::vector<VkFormat>({OBJECT_IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(m_render_buffers->width(), m_render_buffers->height()),
                        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                const auto [w_1, w_2] = window_position_and_size(
                        m_dft_active, m_render_buffers->width(), m_render_buffers->height(), m_frame_size_in_pixels);

                m_draw_rectangle = w_1;

                static_assert(RENDER_BUFFER_COUNT == 1);
                m_image_resolve = std::make_unique<Image>(
                        m_instance->device(), m_instance->graphics_compute_command_pool(),
                        m_instance->graphics_compute_queues()[0], *m_render_buffers, m_draw_rectangle,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT);

                m_renderer->create_buffers(&m_render_buffers->buffers_3d(), m_object_image.get(), m_draw_rectangle);

                m_text->create_buffers(
                        &m_render_buffers->buffers_2d(),
                        Region<2, int>(0, 0, m_render_buffers->width(), m_render_buffers->height()));

                m_convex_hull->create_buffers(&m_render_buffers->buffers_2d(), *m_object_image, m_draw_rectangle);

                m_pencil_sketch->create_buffers(
                        &m_render_buffers->buffers_2d(), m_image_resolve->image(0), *m_object_image, m_draw_rectangle);

                m_optical_flow->create_buffers(
                        &m_render_buffers->buffers_2d(), m_image_resolve->image(0), m_window_ppi, m_draw_rectangle);

                if (w_2)
                {
                        m_dft->create_buffers(
                                &m_render_buffers->buffers_2d(), m_image_resolve->image(0), m_draw_rectangle, *w_2);
                }

                m_camera.resize(m_draw_rectangle.width(), m_draw_rectangle.height());
                m_renderer->set_camera(m_camera.renderer_info());
        }

        [[nodiscard]] VkSemaphore render() const
        {
                static_assert(RENDER_BUFFER_COUNT == 1);
                ASSERT(m_render_buffers->image_views().size() == 1);

                constexpr int INDEX = 0;

                VkSemaphore semaphore = m_renderer->draw(
                        m_instance->graphics_compute_queues()[0], m_instance->graphics_compute_queues()[1], INDEX);

                const vulkan::Queue& graphics_queue = m_instance->graphics_compute_queues()[0];
                const vulkan::Queue& compute_queue = m_instance->compute_queue();

                if (m_pencil_sketch_active)
                {
                        semaphore = m_image_resolve->resolve(graphics_queue, semaphore, INDEX);
                        semaphore = m_pencil_sketch->draw(graphics_queue, semaphore, INDEX);
                }

                if (m_dft_active || m_optical_flow_active)
                {
                        semaphore = m_image_resolve->resolve(graphics_queue, semaphore, INDEX);
                }

                if (m_dft_active)
                {
                        semaphore = m_dft->draw(graphics_queue, semaphore, INDEX);
                }

                if (m_optical_flow_active)
                {
                        semaphore = m_optical_flow->draw(graphics_queue, compute_queue, semaphore, INDEX);
                }

                if (m_convex_hull_active)
                {
                        semaphore = m_convex_hull->draw(graphics_queue, semaphore, INDEX);
                }

                if (m_text_active)
                {
                        semaphore = m_text->draw(graphics_queue, semaphore, INDEX, m_frame_rate.text_data());
                }

                return semaphore;
        }

        //

        void delete_swapchain()
        {
                m_instance->device_wait_idle();

                m_swapchain_resolve.reset();
                delete_buffers();
                m_swapchain.reset();
        }

        void create_swapchain()
        {
                delete_swapchain();

                m_swapchain = std::make_unique<vulkan::Swapchain>(
                        m_instance->surface(), m_instance->device(),
                        std::vector<uint32_t>{
                                m_instance->graphics_compute_queues()[0].family_index(),
                                m_instance->presentation_queue().family_index()},
                        SURFACE_FORMAT, PREFERRED_IMAGE_COUNT, m_present_mode);

                create_buffers(m_swapchain->format(), m_swapchain->width(), m_swapchain->height());

                m_swapchain_resolve = std::make_unique<Swapchain>(
                        m_instance->device(), m_instance->graphics_compute_command_pool(), *m_render_buffers,
                        *m_swapchain);
        }

        [[nodiscard]] bool render_swapchain() const
        {
                const std::optional<uint32_t> image_index = vulkan::acquire_next_image(
                        m_instance->device(), m_swapchain->swapchain(), m_swapchain_image_semaphore);

                if (!image_index)
                {
                        return false;
                }

                const vulkan::Queue& queue = m_instance->graphics_compute_queues()[0];

                VkSemaphore semaphore = render();

                semaphore = m_swapchain_resolve->resolve(queue, m_swapchain_image_semaphore, semaphore, *image_index);

                if (!vulkan::queue_present(
                            semaphore, m_swapchain->swapchain(), *image_index, m_instance->presentation_queue()))
                {
                        return false;
                }

                vulkan::queue_wait_idle(queue);

                return true;
        }

public:
        Impl(const window::WindowID& window, double window_ppi)
                : m_window_ppi(checked_window_ppi(window_ppi)), m_instance(create_instance(window))
        {
                const vulkan::Queue& graphics_compute_queue = m_instance->graphics_compute_queues()[0];
                const vulkan::CommandPool& graphics_compute_command_pool = m_instance->graphics_compute_command_pool();
                const vulkan::Queue& compute_queue = m_instance->compute_queue();
                const vulkan::CommandPool& compute_command_pool = m_instance->compute_command_pool();
                const vulkan::Queue& transfer_queue = m_instance->transfer_queue();
                const vulkan::CommandPool& transfer_command_pool = m_instance->transfer_command_pool();

                m_renderer = gpu::renderer::create_renderer(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING, SAMPLER_ANISOTROPY);

                m_text = gpu::text_writer::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING, m_frame_rate.text_size(), DEFAULT_TEXT_COLOR);

                m_convex_hull = gpu::convex_hull::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, SAMPLE_RATE_SHADING);

                m_pencil_sketch = gpu::pencil_sketch::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING);

                m_dft = gpu::dft::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, SAMPLE_RATE_SHADING);

                m_optical_flow = gpu::optical_flow::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, compute_command_pool,
                        compute_queue, transfer_command_pool, transfer_queue, SAMPLE_RATE_SHADING);

                //

                create_swapchain();

                //

                reset_view_handler();
                clip_plane_hide();
        }

        ~Impl()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                delete_swapchain();
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        void loop(const std::function<void()>& dispatch_events, std::atomic_bool* stop)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                FrameClock::time_point last_frame_time = FrameClock::now();
                while (!(*stop))
                {
                        dispatch_events();

                        if (m_text_active)
                        {
                                m_frame_rate.calculate();
                        }

                        if (!render_swapchain())
                        {
                                create_swapchain();
                                continue;
                        }

                        if (m_renderer->empty())
                        {
                                std::this_thread::sleep_until(last_frame_time + IDLE_MODE_FRAME_DURATION);
                                last_frame_time = FrameClock::now();
                        }
                }
        }

        void send(std::vector<Command>&& commands)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                const auto visitor = [this](const auto& c)
                {
                        command(c);
                };
                for (const view::Command& command : commands)
                {
                        std::visit(visitor, command.data());
                }
        }

        void receive(const std::vector<Info>& info)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                const auto visitor = [this](const auto& d)
                {
                        this->info(d);
                };
                for (const Info& v : info)
                {
                        std::visit(visitor, v.data());
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
