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

#include "camera.h"
#include "frame_rate.h"

#include "com/error.h"
#include "com/log.h"
#include "com/matrix.h"
#include "com/matrix_alg.h"
#include "com/merge.h"
#include "com/print.h"
#include "com/string/vector.h"
#include "com/thread.h"
#include "com/time.h"
#include "com/type/limit.h"
#include "gpu/renderer/opengl/renderer.h"
#include "gpu/renderer/vulkan/renderer.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/render/render_buffer.h"
#include "graphics/vulkan/sync.h"
#include "numerical/linear.h"
#include "show/canvases/opengl/canvas.h"
#include "show/canvases/vulkan/canvas.h"
#include "show/event_queue.h"
#include "window/manage.h"
#include "window/opengl/window.h"
#include "window/vulkan/window.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <type_traits>
#include <vector>

// 2 - double buffering, 3 - triple buffering
constexpr int VULKAN_PREFERRED_IMAGE_COUNT = 2;
// Шейдеры пишут результат в цветовом пространстве RGB, поэтому _SRGB (для результата в sRGB нужен _UNORM).
constexpr VkSurfaceFormatKHR VULKAN_SURFACE_FORMAT = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

constexpr int OPENGL_MINIMUM_SAMPLE_COUNT = 4;

constexpr int VULKAN_MINIMUM_SAMPLE_COUNT = 4;

// supersampling
constexpr bool VULKAN_RENDERER_SAMPLE_SHADING = true;
constexpr bool VULKAN_CANVAS_SAMPLE_SHADING = true;
// anisotropic filtering
constexpr bool VULKAN_SAMPLER_ANISOTROPY = true;

constexpr double IDLE_MODE_FRAME_DURATION_IN_SECONDS = 0.1;

// Это только для начального значения, а далее оно устанавливается командой set_vertical_sync
constexpr vulkan::PresentMode VULKAN_INIT_PRESENT_MODE = vulkan::PresentMode::PreferFast;

constexpr VkFormat OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

namespace
{
// Объекты для std::unique_ptr создаются и удаляются в отдельном потоке. Поток этого
// класса не должен владеть такими объектами. Этот класс нужен, чтобы в явном виде
// не работать с std::unique_ptr.
template <typename T>
class UniquePointerView
{
        const std::unique_ptr<T>* m_pointer = nullptr;

public:
        void set(const std::unique_ptr<T>& pointer)
        {
                m_pointer = &pointer;
        }
        T* operator->() const
        {
                return m_pointer->get();
        }
        operator bool() const
        {
                return static_cast<bool>(*m_pointer);
        }
};

template <GraphicsAndComputeAPI API>
class Impl final : public Show, public WindowEvent
{
        static_assert(API == GraphicsAndComputeAPI::Vulkan || API == GraphicsAndComputeAPI::OpenGL);
        using Renderer = std::conditional_t<API == GraphicsAndComputeAPI::Vulkan, gpu_vulkan::Renderer, gpu_opengl::Renderer>;
        using Window = std::conditional_t<API == GraphicsAndComputeAPI::Vulkan, vulkan::Window, opengl::Window>;
        using Canvas = std::conditional_t<API == GraphicsAndComputeAPI::Vulkan, gpu_vulkan::Canvas, gpu_opengl::Canvas>;

        // Камера и тени рассчитаны на размер объекта 2 и на положение в точке (0, 0, 0).
        static constexpr double OBJECT_SIZE = 2;
        static constexpr vec3 OBJECT_POSITION = vec3(0);

        //

        EventQueue& m_event_queue;
        ShowCallback* const m_callback;
        const WindowID m_parent_window;
        const double m_parent_window_ppi;
        const std::thread::id m_thread_id = std::this_thread::get_id();

        //

        FrameRate m_frame_rate{m_parent_window_ppi};

        Camera m_camera;

        //

        UniquePointerView<Window> m_window;
        UniquePointerView<Renderer> m_renderer;
        UniquePointerView<Canvas> m_canvas;

        //

        int m_window_width = limits<int>::lowest();
        int m_window_height = limits<int>::lowest();

        int m_draw_width = limits<int>::lowest();
        int m_draw_height = limits<int>::lowest();

        int m_mouse_x = 0;
        int m_mouse_y = 0;

        int m_mouse_pressed_x = 0;
        int m_mouse_pressed_y = 0;
        bool m_mouse_pressed = false;
        MouseButton m_mouse_pressed_button;

        bool m_fullscreen_active = false;

        //

        std::function<void(bool)> m_function_set_vertical_sync;

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

                ASSERT(m_function_set_vertical_sync);

                m_function_set_vertical_sync(v);
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

        void window_mouse_pressed(MouseButton button) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!m_mouse_pressed && m_mouse_x < m_draw_width && m_mouse_y < m_draw_height)
                {
                        m_mouse_pressed = true;
                        m_mouse_pressed_button = button;
                        m_mouse_pressed_x = m_mouse_x;
                        m_mouse_pressed_y = m_mouse_y;
                }
        }

        void window_mouse_released(MouseButton button) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_mouse_pressed && button == m_mouse_pressed_button)
                {
                        m_mouse_pressed = false;
                }
        }

        void window_mouse_moved(int x, int y) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_mouse_x = x;
                m_mouse_y = y;

                mouse_move_handler();
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

        void window_resized(int width, int height) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (width <= 0 || height <= 0)
                {
                        error("Window resize error: width = " + to_string(width) + ", height = " + to_string(height));
                }

                m_window_width = width;
                m_window_height = height;

                window_resize_handler();
        }

        //

        void mouse_wheel_handler(int delta)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.scale(m_mouse_x, m_mouse_y, delta);

                m_renderer->set_camera(m_camera.rasterization_info());
        }

        void mouse_move_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!m_mouse_pressed)
                {
                        return;
                }

                int delta_x = m_mouse_x - m_mouse_pressed_x;
                int delta_y = m_mouse_y - m_mouse_pressed_y;

                if (delta_x == 0 && delta_y == 0)
                {
                        return;
                }

                m_mouse_pressed_x = m_mouse_x;
                m_mouse_pressed_y = m_mouse_y;

                switch (m_mouse_pressed_button)
                {
                case MouseButton::Right:
                        m_camera.rotate(-delta_x, -delta_y);
                        break;
                case MouseButton::Left:
                        m_camera.move(vec2(-delta_x, delta_y));
                        break;
                }

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

                if (m_window_width <= 0 || m_window_height <= 0)
                {
                        // Вызов не из обработчика измерения окна (там есть проверка),
                        // а вызов из обработчика включения-выключения ДПФ в то время,
                        // когда окно ещё не готово.
                        return;
                }

                m_draw_width = m_canvas->dft_active() ? m_window_width / 2 : m_window_width;
                m_draw_height = m_window_height;

                resize();

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
                m_window->pull_and_dispath_events();
        }

        void init_window_and_view()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                move_window_to_parent(m_window->system_handle(), m_parent_window);

                for (int i = 1; m_window_width != m_window->width() && m_window_height != m_window->height(); ++i)
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

        //

        void resize();

public:
        Impl(EventQueue& event_queue, ShowCallback* callback, const WindowID& parent_window, double parent_window_ppi)
                : m_event_queue(event_queue),
                  m_callback(callback),
                  m_parent_window(parent_window),
                  m_parent_window_ppi(parent_window_ppi)
        {
        }

        void loop(std::atomic_bool& stop);

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};

//

void render_opengl(opengl::Window& window, gpu_opengl::Renderer& renderer, gpu_opengl::Canvas& canvas, const TextData& text_data)
{
        // Параметр true означает рисование в цветной буфер,
        // параметр false означает рисование в буфер экрана.
        renderer.draw(canvas.pencil_sketch_active());

        canvas.draw(text_data);

        window.display();
}

template <>
void Impl<GraphicsAndComputeAPI::OpenGL>::loop(std::atomic_bool& stop)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::unique_ptr<opengl::Window> window = opengl::create_window(OPENGL_MINIMUM_SAMPLE_COUNT, this);
        std::unique_ptr<gpu_opengl::Renderer> renderer = gpu_opengl::create_renderer(OPENGL_MINIMUM_SAMPLE_COUNT);
        std::unique_ptr<gpu_opengl::Canvas> canvas = gpu_opengl::create_canvas(m_frame_rate.text_size(), m_parent_window_ppi);

        //

        m_window.set(window);
        m_renderer.set(renderer);
        m_canvas.set(canvas);

        m_function_set_vertical_sync = [&](bool v) { window->set_vertical_sync_enabled(v); };

        //

        init_window_and_view();

        //

        double last_frame_time = time_in_seconds();
        while (!stop)
        {
                pull_and_dispatch_all_events();

                m_frame_rate.calculate();

                render_opengl(*window, *renderer, *canvas, m_frame_rate.text_data());

                if (renderer->empty())
                {
                        sleep_this_thread_until(last_frame_time + IDLE_MODE_FRAME_DURATION_IN_SECONDS);
                        last_frame_time = time_in_seconds();
                }
        }
}

template <>
void Impl<GraphicsAndComputeAPI::OpenGL>::resize()
{
        m_renderer->set_size(m_draw_width, m_draw_height);

        int dft_dst_x = (m_window_width & 1) ? (m_draw_width + 1) : m_draw_width;
        int dft_dst_y = 0;

        m_canvas->create_objects(m_window_width, m_window_height, m_renderer->color_buffer(), m_renderer->color_buffer_is_srgb(),
                                 m_renderer->objects(), m_draw_width, m_draw_height, dft_dst_x, dft_dst_y,
                                 m_renderer->frame_buffer_is_srgb());
}

//

void create_swapchain(const vulkan::VulkanInstance& instance, gpu_vulkan::Renderer* renderer, gpu_vulkan::Canvas* canvas,
                      std::unique_ptr<vulkan::Swapchain>* swapchain, std::unique_ptr<vulkan::RenderBuffers>* render_buffers,
                      std::unique_ptr<vulkan::StorageImage>* object_image, vulkan::PresentMode preferred_present_mode,
                      const std::unordered_set<uint32_t>& swapchain_family_indices)
{
        instance.device_wait_idle();

        canvas->delete_buffers();
        renderer->delete_buffers();

        object_image->reset();
        render_buffers->reset();
        swapchain->reset();

        *swapchain =
                std::make_unique<vulkan::Swapchain>(instance.surface(), instance.device(), swapchain_family_indices,
                                                    VULKAN_SURFACE_FORMAT, VULKAN_PREFERRED_IMAGE_COUNT, preferred_present_mode);

        constexpr vulkan::RenderBufferCount buffer_count = vulkan::RenderBufferCount::One;
        *render_buffers =
                vulkan::create_render_buffers(buffer_count, *(swapchain->get()), instance.graphics_command_pool(),
                                              instance.graphics_queues()[0], instance.device(), VULKAN_MINIMUM_SAMPLE_COUNT);

        *object_image = std::make_unique<vulkan::StorageImage>(
                instance.device(), instance.graphics_command_pool(), instance.graphics_queues()[0],
                std::unordered_set({instance.graphics_queues()[0].family_index()}), OBJECT_IMAGE_FORMAT, (*swapchain)->width(),
                (*swapchain)->height());

        renderer->create_buffers(swapchain->get(), &(*render_buffers)->buffers_3d(), object_image->get());

        canvas->create_buffers(swapchain->get(), &(*render_buffers)->buffers_2d(), object_image->get());
}

template <size_t N>
bool render_vulkan(VkSwapchainKHR swapchain, const vulkan::Queue& presentation_queue,
                   const std::array<vulkan::Queue, N>& graphics_queues, VkDevice device, VkSemaphore image_semaphore,
                   const vulkan::RenderBuffers& render_buffers, gpu_vulkan::Renderer& renderer, gpu_vulkan::Canvas& canvas,
                   const TextData& text_data)
{
        static_assert(N >= 1);

        uint32_t image_index;
        if (!vulkan::acquire_next_image(device, swapchain, image_semaphore, &image_index))
        {
                return false;
        }

        VkSemaphore wait_semaphore;

        wait_semaphore = renderer.draw(graphics_queues[0], image_index);
        wait_semaphore = canvas.draw(graphics_queues[0], graphics_queues[0], wait_semaphore, image_index, text_data);
        wait_semaphore = render_buffers.resolve_to_swapchain(graphics_queues[0], image_semaphore, wait_semaphore, image_index);

        if (!vulkan::queue_present(wait_semaphore, swapchain, image_index, presentation_queue))
        {
                return false;
        }

        vulkan::queue_wait_idle(graphics_queues[0]);

        return true;
}

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

template <>
void Impl<GraphicsAndComputeAPI::Vulkan>::loop(std::atomic_bool& stop)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::unique_ptr<vulkan::Window> window = vulkan::create_window(this);

        vulkan::VulkanInstance instance(
                merge<std::string>(gpu_vulkan::Renderer::instance_extensions(), vulkan::Window::instance_extensions()),
                gpu_vulkan::Renderer::device_extensions(),
                merge<vulkan::PhysicalDeviceFeatures>(
                        gpu_vulkan::Renderer::required_device_features(), gpu_vulkan::Canvas::required_device_features(),
                        device_features_sample_shading(VULKAN_MINIMUM_SAMPLE_COUNT,
                                                       VULKAN_RENDERER_SAMPLE_SHADING || VULKAN_CANVAS_SAMPLE_SHADING),
                        device_features_sampler_anisotropy(VULKAN_SAMPLER_ANISOTROPY)),
                {} /*optional_features*/, [w = window.get()](VkInstance i) { return w->create_surface(i); });

        vulkan::Semaphore image_semaphore(instance.device());

        //

        // В последовательности swapchain, а затем renderer и canvas,
        // так как буферы renderer и canvas могут зависеть от swapchain

        std::unique_ptr<vulkan::Swapchain> swapchain;
        std::unique_ptr<vulkan::RenderBuffers> render_buffers;
        std::unique_ptr<vulkan::StorageImage> object_image;

        std::unique_ptr<gpu_vulkan::Renderer> renderer = gpu_vulkan::create_renderer(
                instance, instance.graphics_command_pool(), instance.graphics_queues()[0], instance.transfer_command_pool(),
                instance.transfer_queue(), VULKAN_RENDERER_SAMPLE_SHADING, VULKAN_SAMPLER_ANISOTROPY);

        std::unique_ptr<gpu_vulkan::Canvas> canvas = gpu_vulkan::create_canvas(
                instance, instance.graphics_command_pool(), instance.graphics_queues()[0], instance.transfer_command_pool(),
                instance.transfer_queue(), instance.graphics_queues()[0], VULKAN_CANVAS_SAMPLE_SHADING, m_frame_rate.text_size());

        //

        vulkan::PresentMode present_mode = VULKAN_INIT_PRESENT_MODE;

        const std::unordered_set<uint32_t> swapchain_family_indices = {instance.graphics_queues()[0].family_index(),
                                                                       instance.presentation_queue().family_index()};

        create_swapchain(instance, renderer.get(), canvas.get(), &swapchain, &render_buffers, &object_image, present_mode,
                         swapchain_family_indices);

        //

        m_window.set(window);
        m_renderer.set(renderer);
        m_canvas.set(canvas);

        m_function_set_vertical_sync = [&](bool v) {
                if (v && present_mode != vulkan::PresentMode::PreferSync)
                {
                        present_mode = vulkan::PresentMode::PreferSync;
                        create_swapchain(instance, renderer.get(), canvas.get(), &swapchain, &render_buffers, &object_image,
                                         present_mode, swapchain_family_indices);
                        return;
                }
                if (!v && present_mode != vulkan::PresentMode::PreferFast)
                {
                        present_mode = vulkan::PresentMode::PreferFast;
                        create_swapchain(instance, renderer.get(), canvas.get(), &swapchain, &render_buffers, &object_image,
                                         present_mode, swapchain_family_indices);
                        return;
                }
        };

        //

        init_window_and_view();

        //

        double last_frame_time = time_in_seconds();
        while (!stop)
        {
                pull_and_dispatch_all_events();

                m_frame_rate.calculate();

                if (!render_vulkan(swapchain->swapchain(), instance.presentation_queue(), instance.graphics_queues(),
                                   instance.device(), image_semaphore, *render_buffers, *renderer, *canvas,
                                   m_frame_rate.text_data()))
                {
                        create_swapchain(instance, renderer.get(), canvas.get(), &swapchain, &render_buffers, &object_image,
                                         present_mode, swapchain_family_indices);
                        continue;
                }

                if (renderer->empty())
                {
                        sleep_this_thread_until(last_frame_time + IDLE_MODE_FRAME_DURATION_IN_SECONDS);
                        last_frame_time = time_in_seconds();
                }
        }
}

template <>
void Impl<GraphicsAndComputeAPI::Vulkan>::resize()
{
}

template <typename T>
class ShowThread final : public ShowObject
{
        std::thread::id m_thread_id = std::this_thread::get_id();
        EventQueue m_event_queue;
        std::thread m_thread;
        std::atomic_bool m_stop{false};
        std::atomic_bool m_started{false};

        class EventQueueSetShow final
        {
                EventQueue& m_event_queue;

        public:
                EventQueueSetShow(EventQueue& event_queue, Show& show) : m_event_queue(event_queue)
                {
                        m_event_queue.set_show(&show);
                }
                ~EventQueueSetShow()
                {
                        m_event_queue.set_show(nullptr);
                }
                EventQueueSetShow(const EventQueueSetShow&) = delete;
                EventQueueSetShow(EventQueueSetShow&&) = delete;
                EventQueueSetShow& operator=(const EventQueueSetShow&) = delete;
                EventQueueSetShow& operator=(EventQueueSetShow&&) = delete;
        };

        static void add_to_event_queue(EventQueue& queue, const ShowCreateInfo& info)
        {
                Show& q = queue;

                q.set_ambient(info.ambient.value());
                q.set_diffuse(info.diffuse.value());
                q.set_specular(info.specular.value());
                q.set_background_color(info.background_color.value());
                q.set_default_color(info.default_color.value());
                q.set_wireframe_color(info.wireframe_color.value());
                q.set_default_ns(info.default_ns.value());
                q.show_smooth(info.with_smooth.value());
                q.show_wireframe(info.with_wireframe.value());
                q.show_shadow(info.with_shadow.value());
                q.show_fog(info.with_fog.value());
                q.show_fps(info.with_fps.value());
                q.show_pencil_sketch(info.with_pencil_sketch.value());
                q.show_dft(info.with_dft.value());
                q.set_dft_brightness(info.dft_brightness.value());
                q.set_dft_background_color(info.dft_background_color.value());
                q.set_dft_color(info.dft_color.value());
                q.show_materials(info.with_materials.value());
                q.show_convex_hull_2d(info.with_convex_hull.value());
                q.show_optical_flow(info.with_optical_flow.value());
                q.set_vertical_sync(info.vertical_sync.value());
                q.set_shadow_zoom(info.shadow_zoom.value());
        }

        void thread_function(ShowCallback* callback, WindowID parent_window, double parent_window_ppi)
        {
                try
                {
                        try
                        {
                                try
                                {
                                        T show(m_event_queue, callback, parent_window, parent_window_ppi);

                                        EventQueueSetShow e(m_event_queue, show);

                                        m_started = true;

                                        show.loop(m_stop);

                                        if (!m_stop)
                                        {
                                                error("Thread ended without stop.");
                                        }
                                }
                                catch (...)
                                {
                                        m_started = true;
                                        throw;
                                }
                        }
                        catch (ErrorSourceException& e)
                        {
                                callback->message_error_source(e.msg(), e.src());
                        }
                        catch (std::exception& e)
                        {
                                callback->message_error_fatal(e.what());
                        }
                        catch (...)
                        {
                                callback->message_error_fatal("Unknown Error. Thread ended.");
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in the show thread exception handlers");
                }
        }

        Show& show() override
        {
                return m_event_queue;
        }

        void join_thread()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }

public:
        ShowThread(const ShowCreateInfo& info)
        {
                try
                {
                        try
                        {
                                add_to_event_queue(m_event_queue, info);

                                if (!info.callback.value() || !(info.parent_window_ppi.value() > 0))
                                {
                                        error("Show create information is not complete");
                                }

                                m_thread = std::thread(&ShowThread::thread_function, this, info.callback.value(),
                                                       info.parent_window.value(), info.parent_window_ppi.value());
                        }
                        catch (std::bad_optional_access&)
                        {
                                error("Show create information is not complete");
                        }

                        do
                        {
                                std::this_thread::yield();
                        } while (!m_started);
                }
                catch (...)
                {
                        join_thread();
                        throw;
                }
        }

        ~ShowThread() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                join_thread();
        }

        ShowThread(const ShowThread&);
        ShowThread(ShowThread&&);
        ShowThread& operator=(const ShowThread&);
        ShowThread& operator=(ShowThread&&);
};
}

std::unique_ptr<ShowObject> create_show_object(GraphicsAndComputeAPI api, const ShowCreateInfo& info)
{
        switch (api)
        {
        case GraphicsAndComputeAPI::Vulkan:
                return std::make_unique<ShowThread<Impl<GraphicsAndComputeAPI::Vulkan>>>(info);
        case GraphicsAndComputeAPI::OpenGL:
                return std::make_unique<ShowThread<Impl<GraphicsAndComputeAPI::OpenGL>>>(info);
        }
        error_fatal("Unknown graphics and compute API for show creation");
}
