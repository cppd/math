/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "show.h"

#include "canvas.h"

#include "com/error.h"
#include "com/log.h"
#include "com/merge.h"
#include "com/print.h"
#include "com/time.h"
#include "com/type/limit.h"
#include "gpu/renderer/vulkan/renderer.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/render/render_buffer.h"
#include "graphics/vulkan/sync.h"
#include "show/com/camera.h"
#include "show/com/event_queue.h"
#include "show/com/event_window.h"
#include "show/com/frame_rate.h"
#include "show/com/show_thread.h"
#include "window/manage.h"
#include "window/vulkan/window.h"

constexpr double IDLE_MODE_FRAME_DURATION_IN_SECONDS = 0.1;

// Камера и тени рассчитаны на размер объекта 2 и на положение в точке (0, 0, 0).
constexpr double OBJECT_SIZE = 2;
constexpr vec3 OBJECT_POSITION = vec3(0);

//

// 2 - double buffering, 3 - triple buffering
constexpr int VULKAN_PREFERRED_IMAGE_COUNT = 2;
// Шейдеры пишут результат в цветовом пространстве RGB, поэтому _SRGB (для результата в sRGB нужен _UNORM).
constexpr VkSurfaceFormatKHR VULKAN_SURFACE_FORMAT = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

constexpr int VULKAN_MINIMUM_SAMPLE_COUNT = 4;

// Supersampling
constexpr bool VULKAN_RENDERER_SAMPLE_SHADING = true;
constexpr bool VULKAN_CANVAS_SAMPLE_SHADING = true;
// Anisotropic filtering
constexpr bool VULKAN_SAMPLER_ANISOTROPY = true;

// Это только для начального значения, а далее оно устанавливается командой set_vertical_sync
constexpr vulkan::PresentMode VULKAN_DEFAULT_PRESENT_MODE = vulkan::PresentMode::PreferFast;

constexpr VkFormat VULKAN_OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

//

namespace
{
std::vector<vulkan::PhysicalDeviceFeatures> device_features_sample_shading(int sample_count, bool sample_shading)
{
        if (sample_count > 1 && sample_shading)
        {
                return {vulkan::PhysicalDeviceFeatures::SampleRateShading};
        }
        else
        {
                return {};
        }
}

std::vector<vulkan::PhysicalDeviceFeatures> device_features_sampler_anisotropy(bool sampler_anisotropy)
{
        if (sampler_anisotropy)
        {
                return {vulkan::PhysicalDeviceFeatures::SamplerAnisotropy};
        }
        else
        {
                return {};
        }
}

class Impl final : public Show, public WindowEvent
{
        EventQueue& m_event_queue;
        EventWindow<vulkan::Window> m_event_window;
        ShowCallback* const m_callback;
        const WindowID m_parent_window;
        const double m_parent_window_ppi;
        const std::thread::id m_thread_id = std::this_thread::get_id();

        FrameRate m_frame_rate{m_parent_window_ppi};
        Camera m_camera;

        int m_draw_width = limits<int>::lowest();
        int m_draw_height = limits<int>::lowest();
        bool m_fullscreen_active = false;

        //

        vulkan::PresentMode m_present_mode = VULKAN_DEFAULT_PRESENT_MODE;

        // В последовательности swapchain, а затем renderer и canvas,
        // так как буферы renderer и canvas могут зависеть от swapchain
        std::unique_ptr<vulkan::Window> m_window;
        std::unique_ptr<vulkan::VulkanInstance> m_instance;
        std::unique_ptr<vulkan::Semaphore> m_image_semaphore;
        std::unique_ptr<vulkan::Swapchain> m_swapchain;
        std::unique_ptr<vulkan::RenderBuffers> m_render_buffers;
        std::unique_ptr<vulkan::StorageImage> m_object_image;
        std::unique_ptr<gpu_vulkan::Renderer> m_renderer;
        std::unique_ptr<gpu_vulkan::Canvas> m_canvas;

        //

        void add_object(const std::shared_ptr<const Obj<3>>& obj_ptr, int id, int scale_id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!obj_ptr)
                {
                        error("Null object received");
                }
                m_renderer->object_add(obj_ptr.get(), OBJECT_SIZE, OBJECT_POSITION, id, scale_id);
                m_callback->object_loaded(id);
        }

        void delete_object(int id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->object_delete(id);
        }

        void show_object(int id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->object_show(id);
        }

        void delete_all_objects() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->object_delete_all();

                reset_view_handler();
        }

        void reset_view() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                reset_view_handler();
        }

        void set_ambient(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                Color light = Color(v);
                m_renderer->set_light_a(light);
        }

        void set_diffuse(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                Color light = Color(v);
                m_renderer->set_light_d(light);
        }

        void set_specular(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                Color light = Color(v);
                m_renderer->set_light_s(light);
        }

        void set_background_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_background_color(c);

                bool background_is_dark = c.luminance() <= 0.5;
                m_canvas->set_text_color(background_is_dark ? Color(1) : Color(0));
        }

        void set_default_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_default_color(c);
        }

        void set_wireframe_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_wireframe_color(c);
        }

        void set_default_ns(double ns) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_default_ns(ns);
        }

        void show_smooth(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_show_smooth(v);
        }

        void show_wireframe(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_show_wireframe(v);
        }

        void show_shadow(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_show_shadow(v);
        }

        void show_fog(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_show_fog(v);
        }

        void show_materials(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_show_materials(v);
        }

        void show_fps(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_text_active(v);
        }

        void show_pencil_sketch(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_pencil_sketch_active(v);
        }

        void show_dft(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_canvas->dft_active() != v)
                {
                        m_canvas->set_dft_active(v);
                        window_resize_handler();
                }
        }

        void set_dft_brightness(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_dft_brightness(v);
        }

        void set_dft_background_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_dft_background_color(c);
        }

        void set_dft_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_dft_color(c);
        }

        void show_convex_hull_2d(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_convex_hull_active(v);
        }

        void show_optical_flow(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_canvas->set_optical_flow_active(v);
        }

        void parent_resized() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!m_fullscreen_active)
                {
                        set_size_to_parent(m_window->system_handle(), m_parent_window);
                }
        }

        void mouse_wheel(double delta) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // Для полноэкранного режима обрабатывается в функции window_mouse_wheel
                if (!m_fullscreen_active)
                {
                        mouse_wheel_handler(delta);
                }
        }

        void toggle_fullscreen() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!m_fullscreen_active)
                {
                        make_window_fullscreen(m_window->system_handle());
                        m_fullscreen_active = true;
                }
                else
                {
                        move_window_to_parent(m_window->system_handle(), m_parent_window);
                        m_fullscreen_active = false;
                        set_focus(m_window->system_handle());
                }
        }

        void set_vertical_sync(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                set_vertical_sync_swapchain(v);
        }

        void set_shadow_zoom(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_shadow_zoom(v);
        }

        RayCameraInfo camera_information() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread_id);

                return m_camera.ray_info();
        }

        double object_size() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread_id);

                return OBJECT_SIZE;
        }

        vec3 object_position() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread_id);

                return OBJECT_POSITION;
        }

        //

        void window_keyboard_pressed(KeyboardButton button) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                switch (button)
                {
                case KeyboardButton::F11:
                        toggle_fullscreen();
                        break;
                case KeyboardButton::Escape:
                        if (m_fullscreen_active)
                        {
                                toggle_fullscreen();
                        }
                        break;
                }
        }

        void window_mouse_pressed(MouseButton /*button*/) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void window_mouse_released(MouseButton /*button*/) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void window_mouse_moved(int /*x*/, int /*y*/) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                bool changed = false;

                const PressedMouseButton& right = m_event_window.pressed_mouse_button(MouseButton::Right);
                if (right.pressed && right.pressed_x < m_draw_width && right.pressed_y < m_draw_height &&
                    (right.delta_x != 0 || right.delta_y != 0))
                {
                        m_camera.rotate(-right.delta_x, -right.delta_y);
                        changed = true;
                }

                const PressedMouseButton& left = m_event_window.pressed_mouse_button(MouseButton::Left);
                if (left.pressed && left.pressed_x < m_draw_width && left.pressed_y < m_draw_height &&
                    (left.delta_x != 0 || left.delta_y != 0))
                {
                        m_camera.move(vec2(-left.delta_x, left.delta_y));
                        changed = true;
                }

                if (changed)
                {
                        m_renderer->set_camera(m_camera.rasterization_info());
                }
        }

        void window_mouse_wheel(int delta) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // Для режима встроенного окна обработка колеса мыши происходит
                // в функции mouse_wheel, так как на Винде не приходит
                // это сообщение для дочернего окна.
                if (m_fullscreen_active)
                {
                        mouse_wheel_handler(delta);
                }
        }

        void window_resized(int /*width*/, int /*height*/) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                window_resize_handler();
        }

        //

        void mouse_wheel_handler(int delta)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.scale(m_event_window.mouse_x(), m_event_window.mouse_y(), delta);

                m_renderer->set_camera(m_camera.rasterization_info());
        }

        void reset_view_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.reset(vec3(1, 0, 0), vec3(0, 1, 0), 1, vec2(0, 0));

                m_renderer->set_camera(m_camera.rasterization_info());
        }

        void window_resize_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_event_window.window_width() != m_window->width() || m_event_window.window_height() != m_window->height())
                {
                        // Вызов не из обработчика измерения окна (там есть проверка),
                        // а вызов из обработчика включения-выключения ДПФ в то время,
                        // когда окно ещё не готово.
                        return;
                }

                m_draw_width = m_canvas->dft_active() ? m_window->width() / 2 : m_window->width();
                m_draw_height = m_window->height();

                m_camera.resize(m_draw_width, m_draw_height);
                m_renderer->set_camera(m_camera.rasterization_info());
        }

        //

        void pull_and_dispatch_all_events()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // Вначале команды, потом сообщения окна, потому что в командах
                // могут быть действия с окном, а в событиях окна нет комманд
                m_event_queue.pull_and_dispatch_events(*this);
                m_event_window.pull_and_dispath_events(*this);
        }

        void init_window_and_view()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                move_window_to_parent(m_window->system_handle(), m_parent_window);

                for (int i = 1;
                     m_event_window.window_width() != m_window->width() && m_event_window.window_height() != m_window->height();
                     ++i)
                {
                        if (i > 10)
                        {
                                error("Failed to receive the resize window event for the window size (" +
                                      to_string(m_window->width()) + ", " + to_string(m_window->height()) + ")");
                        }
                        pull_and_dispatch_all_events();
                }

                if (m_draw_width <= 0 || m_draw_height <= 0)
                {
                        error("Draw size error (" + to_string(m_draw_width) + ", " + to_string(m_draw_height) + ")");
                }

                reset_view_handler();
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

        void create_swapchain()
        {
                m_instance->device_wait_idle();

                m_canvas->delete_buffers();
                m_renderer->delete_buffers();

                m_object_image.reset();
                m_render_buffers.reset();
                m_swapchain.reset();

                const std::unordered_set<uint32_t> swapchain_family_indices = {m_instance->graphics_queues()[0].family_index(),
                                                                               m_instance->presentation_queue().family_index()};

                m_swapchain =
                        std::make_unique<vulkan::Swapchain>(m_instance->surface(), m_instance->device(), swapchain_family_indices,
                                                            VULKAN_SURFACE_FORMAT, VULKAN_PREFERRED_IMAGE_COUNT, m_present_mode);

                constexpr vulkan::RenderBufferCount buffer_count = vulkan::RenderBufferCount::One;

                m_render_buffers = vulkan::create_render_buffers(buffer_count, *m_swapchain, m_instance->graphics_command_pool(),
                                                                 m_instance->graphics_queues()[0], m_instance->device(),
                                                                 VULKAN_MINIMUM_SAMPLE_COUNT);

                m_object_image = std::make_unique<vulkan::StorageImage>(
                        m_instance->device(), m_instance->graphics_command_pool(), m_instance->graphics_queues()[0],
                        std::unordered_set({m_instance->graphics_queues()[0].family_index()}), VULKAN_OBJECT_IMAGE_FORMAT,
                        m_swapchain->width(), m_swapchain->height());

                m_renderer->create_buffers(m_swapchain.get(), &m_render_buffers->buffers_3d(), m_object_image.get());

                m_canvas->create_buffers(m_swapchain.get(), &m_render_buffers->buffers_2d(), m_object_image.get());
        }

        bool render(const TextData& text_data)
        {
                static_assert(std::remove_reference_t<decltype(m_instance->graphics_queues())>().size() > 0);

                uint32_t image_index;
                if (!vulkan::acquire_next_image(m_instance->device(), m_swapchain->swapchain(), *m_image_semaphore, &image_index))
                {
                        return false;
                }

                VkSemaphore wait_semaphore;

                const vulkan::Queue& graphics_queue = m_instance->graphics_queues()[0];

                wait_semaphore = m_renderer->draw(graphics_queue, image_index);
                wait_semaphore = m_canvas->draw(graphics_queue, graphics_queue, wait_semaphore, image_index, text_data);
                wait_semaphore =
                        m_render_buffers->resolve_to_swapchain(graphics_queue, *m_image_semaphore, wait_semaphore, image_index);

                if (!vulkan::queue_present(wait_semaphore, m_swapchain->swapchain(), image_index,
                                           m_instance->presentation_queue()))
                {
                        return false;
                }

                vulkan::queue_wait_idle(graphics_queue);

                return true;
        }

public:
        Impl(EventQueue& event_queue, ShowCallback* callback, const WindowID& parent_window, double parent_window_ppi)
                : m_event_queue(event_queue),
                  m_callback(callback),
                  m_parent_window(parent_window),
                  m_parent_window_ppi(parent_window_ppi)
        {
                m_present_mode = VULKAN_DEFAULT_PRESENT_MODE;

                m_window = vulkan::create_window();

                {
                        const std::vector<std::string> required_instance_extensions = merge<std::string>(
                                gpu_vulkan::Renderer::instance_extensions(), vulkan::Window::instance_extensions());

                        const std::vector<std::string> required_device_extensions =
                                merge<std::string>(gpu_vulkan::Renderer::device_extensions());

                        const std::vector<vulkan::PhysicalDeviceFeatures> features_sample_shading =
                                device_features_sample_shading(VULKAN_MINIMUM_SAMPLE_COUNT,
                                                               VULKAN_RENDERER_SAMPLE_SHADING || VULKAN_CANVAS_SAMPLE_SHADING);

                        const std::vector<vulkan::PhysicalDeviceFeatures> features_sampler_anisotropy =
                                device_features_sampler_anisotropy(VULKAN_SAMPLER_ANISOTROPY);

                        const std::vector<vulkan::PhysicalDeviceFeatures> required_features =
                                merge<vulkan::PhysicalDeviceFeatures>(gpu_vulkan::Renderer::required_device_features(),
                                                                      gpu_vulkan::Canvas::required_device_features(),
                                                                      features_sample_shading, features_sampler_anisotropy);

                        const std::vector<vulkan::PhysicalDeviceFeatures> optional_features = {};

                        const std::function<VkSurfaceKHR(VkInstance)> create_surface = [&](VkInstance i) {
                                return m_window->create_surface(i);
                        };

                        m_instance =
                                std::make_unique<vulkan::VulkanInstance>(required_instance_extensions, required_device_extensions,
                                                                         required_features, optional_features, create_surface);
                }

                m_image_semaphore = std::make_unique<vulkan::Semaphore>(m_instance->device());

                m_renderer = gpu_vulkan::create_renderer(*m_instance, m_instance->graphics_command_pool(),
                                                         m_instance->graphics_queues()[0], m_instance->transfer_command_pool(),
                                                         m_instance->transfer_queue(), VULKAN_RENDERER_SAMPLE_SHADING,
                                                         VULKAN_SAMPLER_ANISOTROPY);

                m_canvas = gpu_vulkan::create_canvas(*m_instance, m_instance->graphics_command_pool(),
                                                     m_instance->graphics_queues()[0], m_instance->transfer_command_pool(),
                                                     m_instance->transfer_queue(), m_instance->graphics_queues()[0],
                                                     VULKAN_CANVAS_SAMPLE_SHADING, m_frame_rate.text_size());

                create_swapchain();

                //

                m_event_window.set_window(*m_window);

                init_window_and_view();
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // В последовательности canvas, renderer, а затем swapchain,
                // так как буферы renderer и canvas могут зависеть от swapchain
                // m_canvas.reset();
                // m_renderer.reset();
                // m_object_image.reset();
                // m_render_buffers.reset();
                // m_swapchain.reset();
                // m_image_semaphore.reset();
                // m_instance.reset();
                // m_window.reset();
        }

        void loop(std::atomic_bool& stop)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                double last_frame_time = time_in_seconds();
                while (!stop)
                {
                        pull_and_dispatch_all_events();

                        m_frame_rate.calculate();

                        if (!render(m_frame_rate.text_data()))
                        {
                                create_swapchain();
                                continue;
                        }

                        if (m_renderer->empty())
                        {
                                sleep_this_thread_until(last_frame_time + IDLE_MODE_FRAME_DURATION_IN_SECONDS);
                                last_frame_time = time_in_seconds();
                        }
                }
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

std::unique_ptr<ShowObject> create_show_object_vulkan(const ShowCreateInfo& info)
{
        return std::make_unique<ShowThread<Impl>>(info);
}
