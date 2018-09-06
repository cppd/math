/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include "fps.h"

#include "com/error.h"
#include "com/log.h"
#include "com/mat.h"
#include "com/mat_alg.h"
#include "com/math.h"
#include "com/print.h"
#include "com/thread.h"
#include "com/time.h"
#include "gpu_2d/convex_hull/convex_hull_2d.h"
#include "gpu_2d/dft/show/dft_show.h"
#include "gpu_2d/optical_flow/optical_flow.h"
#include "gpu_2d/pencil/pencil.h"
#include "gpu_2d/text/text.h"
#include "graphics/opengl/objects.h"
#include "graphics/opengl/window.h"
#include "graphics/vulkan/window.h"
#include "numerical/linear.h"
#include "show/event_queue.h"
#include "show/renderers/opengl/renderer.h"
#include "show/renderers/vulkan/renderer.h"
#include "window/window_prop.h"

#include <chrono>
#include <unordered_map>
#include <vector>

constexpr double ZOOM_BASE = 1.1;
constexpr double ZOOM_EXP_MIN = -50;
constexpr double ZOOM_EXP_MAX = 100;

constexpr const char FPS_STRING[] = "FPS: ";
constexpr double FPS_TEXT_SIZE_IN_POINTS = 9.0;
constexpr double FPS_TEXT_STEP_Y_IN_POINTS = 1.3 * FPS_TEXT_SIZE_IN_POINTS;
constexpr double FPS_TEXT_START_X_IN_POINTS = 5;
constexpr double FPS_TEXT_START_Y_IN_POINTS = FPS_TEXT_STEP_Y_IN_POINTS;

constexpr std::chrono::milliseconds IDLE_MODE_FRAME_DURATION(100);

namespace
{
int points_to_pixels(double points, double dpi)
{
        return std::round(points / 72.0 * dpi);
}

#if 0
int object_under_mouse(int mouse_x, int mouse_y, int window_height, const TextureR32I& tex)
{
        int x = mouse_x;
        int y = window_height - mouse_y - 1;
        std::array<GLint, 1> v;
        tex.get_texture_sub_image(x, y, 1, 1, &v);
        return v[0];
}
#endif

void make_fullscreen(bool fullscreen, WindowID window, WindowID parent)
{
        if (fullscreen)
        {
                make_window_fullscreen(window);
        }
        else
        {
                move_window_to_parent(window, parent);
        }
        set_focus(window);
}

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

class FPSText
{
        Text m_text;
        FPS m_fps;
        std::vector<std::string> m_fps_text;

public:
        FPSText(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
                : m_text(size, step_y, start_x, start_y, color, matrix), m_fps_text({FPS_STRING, ""})
        {
        }

        void set_color(const Color& color) const
        {
                m_text.set_color(color);
        }

        void set_matrix(const mat4& matrix) const
        {
                m_text.set_matrix(matrix);
        }

        void draw()
        {
                m_fps_text[1] = to_string(m_fps.calculate());
                m_text.draw(m_fps_text);
        }
};

template <GraphicsAndComputeAPI API>
class ShowObject final : public EventQueue, public WindowEvent
{
        static_assert(API == GraphicsAndComputeAPI::Vulkan || API == GraphicsAndComputeAPI::OpenGL);
        using Renderer = std::conditional_t<API == GraphicsAndComputeAPI::Vulkan, VulkanRenderer, OpenGLRenderer>;
        using Window = std::conditional_t<API == GraphicsAndComputeAPI::Vulkan, VulkanWindow, OpenGLWindow>;

        // Камера и тени рассчитаны на размер объекта 2 и на положение в точке (0, 0, 0).
        static constexpr double OBJECT_SIZE = 2;
        static constexpr vec3 OBJECT_POSITION = vec3(0);

        //

        IShowCallback* const m_callback;
        const WindowID m_parent_window;
        const double m_parent_window_dpi;
        std::thread m_thread;
        std::atomic_bool m_stop{false};

        //

        Camera m_camera;

        //

        UniquePointerView<Window> m_window;
        UniquePointerView<Renderer> m_renderer;

        UniquePointerView<FPSText> m_fps_text;
        UniquePointerView<DFTShow> m_dft_show;
        UniquePointerView<ConvexHull2D> m_convex_hull;
        UniquePointerView<OpticalFlow> m_optical_flow;
        UniquePointerView<PencilEffect> m_pencil_effect;

        //

        int m_window_width = 0;
        int m_window_height = 0;

        int m_draw_width = 0;
        int m_draw_height = 0;

        int m_mouse_x = 0;
        int m_mouse_y = 0;

        int m_mouse_pressed_x = 0;
        int m_mouse_pressed_y = 0;
        bool m_mouse_pressed = false;
        MouseButton m_mouse_pressed_button;

        vec2 m_window_center = vec2(0, 0);
        double m_zoom_exponent = 0;

        bool m_matrix_changed = false;
        bool m_window_size_changed = false;
        bool m_dft_show_active_changed = false;

        bool m_fullscreen_active = false;
        bool m_fps_text_active = true;

        // Неважно, какие тут будут значения при инициализации,
        // так как при создании окна задаются начальные параметры

        bool m_pencil_effect_active;
        bool m_dft_show_active;
        bool m_convex_hull_active;
        bool m_optical_flow_active;

        double m_dft_show_brightness;
        Color m_dft_show_background_color;
        Color m_dft_show_color;

        Color m_fps_text_color;

        //

        void direct_add_object(const std::shared_ptr<const Obj<3>>& obj_ptr, int id, int scale_id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                if (!obj_ptr)
                {
                        error("Null object received");
                }
                m_renderer->object_add(obj_ptr.get(), OBJECT_SIZE, OBJECT_POSITION, id, scale_id);
                m_callback->object_loaded(id);
        }

        void direct_delete_object(int id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->object_delete(id);
        }

        void direct_show_object(int id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->object_show(id);
        }

        void direct_delete_all_objects() override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->object_delete_all();

                reset_view_handler();
        }

        void direct_reset_view() override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                reset_view_handler();
        }

        void direct_set_ambient(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                Color light = Color(v);
                m_renderer->set_light_a(light);
        }

        void direct_set_diffuse(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                Color light = Color(v);
                m_renderer->set_light_d(light);
        }

        void direct_set_specular(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                Color light = Color(v);
                m_renderer->set_light_s(light);
        }

        void direct_set_background_color_rgb(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                glClearColor(c.red(), c.green(), c.blue(), 1);
                m_renderer->set_background_color(c);

                bool background_is_dark = c.luminance() <= 0.5;
                m_fps_text_color = background_is_dark ? Color(1) : Color(0);
                if (m_fps_text)
                {
                        m_fps_text->set_color(m_fps_text_color);
                }
        }

        void direct_set_default_color_rgb(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_default_color(c);
        }

        void direct_set_wireframe_color_rgb(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_wireframe_color(c);
        }

        void direct_set_default_ns(double ns) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_default_ns(ns);
        }

        void direct_show_smooth(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_show_smooth(v);
        }

        void direct_show_wireframe(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_show_wireframe(v);
        }

        void direct_show_shadow(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_show_shadow(v);
        }

        void direct_show_fog(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_show_fog(v);
        }

        void direct_show_materials(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_show_materials(v);
        }

        void direct_show_effect(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_pencil_effect_active = v;
        }

        void direct_show_dft(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_dft_show_active_changed = (v != m_dft_show_active);

                m_dft_show_active = v;
        }

        void direct_set_dft_brightness(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_dft_show_brightness = v;
                if (m_dft_show)
                {
                        m_dft_show->set_brightness(v);
                }
        }

        void direct_set_dft_background_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_dft_show_background_color = c;
                if (m_dft_show)
                {
                        m_dft_show->set_background_color(c);
                }
        }

        void direct_set_dft_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_dft_show_color = c;
                if (m_dft_show)
                {
                        m_dft_show->set_color(c);
                }
        }

        void direct_show_convex_hull_2d(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_convex_hull_active = v;
                if (m_convex_hull)
                {
                        m_convex_hull->reset_timer();
                }
        }

        void direct_show_optical_flow(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_optical_flow_active = v;
                if (m_optical_flow)
                {
                        m_optical_flow->reset();
                }
        }

        void direct_parent_resized() override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                if (!m_fullscreen_active)
                {
                        set_size_to_parent(m_window->get_system_handle(), m_parent_window);
                }
        }

        void direct_mouse_wheel(double delta) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                // Для полноэкранного режима обрабатывается в функции window_mouse_wheel
                if (!m_fullscreen_active)
                {
                        mouse_wheel_handler(delta);
                }
        }

        void direct_toggle_fullscreen() override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_fullscreen_active = !m_fullscreen_active;
                make_fullscreen(m_fullscreen_active, m_window->get_system_handle(), m_parent_window);
        }

        void direct_set_vertical_sync([[maybe_unused]] bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                if constexpr (API == GraphicsAndComputeAPI::OpenGL)
                {
                        m_window->set_vertical_sync_enabled(v);
                }
        }

        void direct_set_shadow_zoom(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_renderer->set_shadow_zoom(v);
        }

        //

        void camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width, int* paint_width,
                                int* paint_height) const override
        {
                ASSERT(std::this_thread::get_id() != m_thread.get_id());

                m_camera.camera_information(camera_up, camera_direction, view_center, view_width, paint_width, paint_height);
        }

        vec3 light_direction() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread.get_id());

                return m_camera.light_direction();
        }

        double object_size() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread.get_id());

                return OBJECT_SIZE;
        }

        vec3 object_position() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread.get_id());

                return OBJECT_POSITION;
        }

        //

        void window_keyboard_pressed(KeyboardButton button) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

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
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

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
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                if (m_mouse_pressed && button == m_mouse_pressed_button)
                {
                        m_mouse_pressed = false;
                }
        }

        void window_mouse_moved(int x, int y) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_mouse_x = x;
                m_mouse_y = y;

                mouse_move_handler();
        }

        void window_mouse_wheel(int delta) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                // Для режима встроенного окна обработка колеса мыши происходит
                // в функции direct_mouse_wheel, так как на Винде не приходит
                // это сообщение для дочернего окна.
                if (m_fullscreen_active)
                {
                        mouse_wheel_handler(delta);
                }
        }

        void window_resized(int width, int height) override
        {
                ASSERT(std::this_thread::get_id() == m_thread.get_id());

                m_window_size_changed = (m_window_width != width) || (m_window_height != height);

                m_window_width = width;
                m_window_height = height;
        }

        //

        void mouse_wheel_handler(int delta);
        void mouse_move_handler();
        void reset_view_handler();

        //

        void loop();
        void loop_thread();

public:
        ShowObject(const ShowCreateInfo& info) try : m_callback(info.callback.value()),
                                                     m_parent_window(info.parent_window.value()),
                                                     m_parent_window_dpi(info.parent_window_dpi.value())
        {
                ASSERT(m_callback);
                ASSERT(m_parent_window_dpi > 0);

                reset_view();

                set_ambient(info.ambient.value());
                set_diffuse(info.diffuse.value());
                set_specular(info.specular.value());
                set_background_color_rgb(info.background_color_rgb.value());
                set_default_color_rgb(info.default_color_rgb.value());
                set_wireframe_color_rgb(info.wireframe_color_rgb.value());
                set_default_ns(info.default_ns.value());
                show_smooth(info.with_smooth.value());
                show_wireframe(info.with_wireframe.value());
                show_shadow(info.with_shadow.value());
                show_fog(info.with_fog.value());
                show_effect(info.with_effect.value());
                show_dft(info.with_dft.value());
                set_dft_brightness(info.dft_brightness.value());
                set_dft_background_color(info.dft_background_color.value());
                set_dft_color(info.dft_color.value());
                show_materials(info.with_materials.value());
                show_convex_hull_2d(info.with_convex_hull.value());
                show_optical_flow(info.with_optical_flow.value());
                set_vertical_sync(info.vertical_sync.value());
                set_shadow_zoom(info.shadow_zoom.value());

                m_thread = std::thread(&ShowObject::loop_thread, this);
        }
        catch (std::bad_optional_access&)
        {
                error_fatal("Show create information is not complete");
        }

        ~ShowObject() override
        {
                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }

        ShowObject(const ShowObject&) = delete;
        ShowObject(ShowObject&&) = delete;
        ShowObject& operator=(const ShowObject&) = delete;
        ShowObject& operator=(ShowObject&&) = delete;
};

template <GraphicsAndComputeAPI API>
void ShowObject<API>::reset_view_handler()
{
        m_zoom_exponent = 0;
        m_window_center = vec2(0, 0);
        m_camera.set(vec3(1, 0, 0), vec3(0, 1, 0));

        //

        m_matrix_changed = true;
}

template <GraphicsAndComputeAPI API>
void ShowObject<API>::mouse_wheel_handler(int delta)
{
        if (!(m_mouse_x < m_draw_width && m_mouse_y < m_draw_height))
        {
                return;
        }
        if ((delta < 0 && m_zoom_exponent <= ZOOM_EXP_MIN) || (delta > 0 && m_zoom_exponent >= ZOOM_EXP_MAX) || delta == 0)
        {
                return;
        }

        m_zoom_exponent += delta;

        vec2 mouse_local(m_mouse_x - m_draw_width * 0.5, m_draw_height * 0.5 - m_mouse_y);
        vec2 mouse_global(mouse_local + m_window_center);
        // Формула
        //   new_center = old_center + (mouse_global * zoom_r - mouse_global)
        //   -> center = center + mouse_global * zoom_r - mouse_global
        //   -> center += mouse_global * (zoom_r - 1)
        m_window_center += mouse_global * (std::pow(ZOOM_BASE, delta) - 1);

        //

        m_matrix_changed = true;
}

template <GraphicsAndComputeAPI API>
void ShowObject<API>::mouse_move_handler()
{
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
                m_window_center -= vec2(delta_x, -delta_y);
                break;
        }

        //

        m_matrix_changed = true;
}

template <GraphicsAndComputeAPI API>
void ShowObject<API>::loop()
{
        ASSERT(std::this_thread::get_id() == m_thread.get_id());

        std::unique_ptr<Window> window;
        std::unique_ptr<Renderer> renderer;
        if constexpr (API == GraphicsAndComputeAPI::Vulkan)
        {
                window = create_vulkan_window(this);
                renderer = create_vulkan_renderer(VulkanWindow::instance_extensions(), [w = window.get()](VkInstance instance) {
                        return w->create_surface(instance);
                });
        }
        if constexpr (API == GraphicsAndComputeAPI::OpenGL)
        {
                window = create_opengl_window(this);
                renderer = create_opengl_renderer();
        }

        move_window_to_parent(window->get_system_handle(), m_parent_window);

        double default_ortho_scale = 1;

        std::chrono::steady_clock::time_point last_frame_time = std::chrono::steady_clock::now();

        std::unique_ptr<FPSText> fps_text;
        std::unique_ptr<DFTShow> dft_show;
        std::unique_ptr<ConvexHull2D> convex_hull;
        std::unique_ptr<OpticalFlow> optical_flow;
        std::unique_ptr<PencilEffect> pencil_effect;

        m_window.set(window);
        m_renderer.set(renderer);

        m_convex_hull.set(convex_hull);
        m_optical_flow.set(optical_flow);
        m_pencil_effect.set(pencil_effect);
        m_dft_show.set(dft_show);
        m_fps_text.set(fps_text);

        while (true)
        {
                if (m_stop)
                {
#if defined(_WIN32)
                        // Без этого вызова почему-то зависает деструктор окна SFML на Винде,
                        // если это окно встроено в родительское окно.
                        change_window_style_not_child(window->get_system_handle());
#endif

                        return;
                }

                // Вначале команды, потом сообщения окна, потому что в командах
                // могут быть действия с окном, а в событиях окна нет комманд
                this->pull_and_dispatch_events();
                window->pull_and_dispath_events();

                if (m_window_size_changed || m_dft_show_active_changed)
                {
                        m_window_size_changed = false;
                        m_dft_show_active_changed = false;

                        m_matrix_changed = true;

                        //

                        m_draw_width =
                                API == GraphicsAndComputeAPI::OpenGL && m_dft_show_active ? m_window_width / 2 : m_window_width;
                        m_draw_height = m_window_height;

                        default_ortho_scale = 2.0 / std::min(m_draw_width, m_draw_height);

                        renderer->set_size(m_draw_width, m_draw_height);

                        if constexpr (API == GraphicsAndComputeAPI::OpenGL)
                        {
                                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху

                                double left = 0;
                                double right = m_window_width;
                                double bottom = m_window_height;
                                double top = 0;
                                double near = 1;
                                double far = -1;
                                mat4 matrix = Renderer::ortho(left, right, bottom, top, near, far) * scale<double>(1, 1, 0);

                                pencil_effect = std::make_unique<PencilEffect>(
                                        renderer->color_buffer(), renderer->color_buffer_is_srgb(), renderer->objects(), matrix);

                                int dft_dst_x = (m_window_width & 1) ? (m_draw_width + 1) : m_draw_width;
                                int dft_dst_y = 0;
                                dft_show = std::make_unique<DFTShow>(m_draw_width, m_draw_height, dft_dst_x, dft_dst_y, matrix,
                                                                     renderer->frame_buffer_is_srgb(), m_dft_show_brightness,
                                                                     m_dft_show_background_color, m_dft_show_color);

                                optical_flow = std::make_unique<OpticalFlow>(m_draw_width, m_draw_height, matrix);

                                convex_hull = std::make_unique<ConvexHull2D>(renderer->objects(), matrix);

                                if (fps_text)
                                {
                                        fps_text->set_matrix(matrix);
                                }
                                else
                                {
                                        int text_size = points_to_pixels(FPS_TEXT_SIZE_IN_POINTS, m_parent_window_dpi);
                                        int text_step_y = points_to_pixels(FPS_TEXT_STEP_Y_IN_POINTS, m_parent_window_dpi);
                                        int text_start_x = points_to_pixels(FPS_TEXT_START_X_IN_POINTS, m_parent_window_dpi);
                                        int text_start_y = points_to_pixels(FPS_TEXT_START_Y_IN_POINTS, m_parent_window_dpi);
                                        fps_text = std::make_unique<FPSText>(text_size, text_step_y, text_start_x, text_start_y,
                                                                             m_fps_text_color, matrix);
                                }
                        }
                }

                if (m_matrix_changed)
                {
                        m_matrix_changed = false;

                        vec3 camera_up, camera_direction, light_up, light_direction;

                        m_camera.get(&camera_up, &camera_direction, &light_up, &light_direction);

                        mat4 shadow_projection_matrix = Renderer::ortho(-1, 1, -1, 1, 1, -1);
                        mat4 shadow_view_matrix = look_at(vec3(0, 0, 0), light_direction, light_up);

                        double ortho_scale = std::pow(ZOOM_BASE, -m_zoom_exponent) * default_ortho_scale;
                        double left = ortho_scale * (m_window_center[0] - 0.5 * m_draw_width);
                        double right = ortho_scale * (m_window_center[0] + 0.5 * m_draw_width);
                        double bottom = ortho_scale * (m_window_center[1] - 0.5 * m_draw_height);
                        double top = ortho_scale * (m_window_center[1] + 0.5 * m_draw_height);
                        double near = 1;
                        double far = -1;
                        mat4 projection_matrix = Renderer::ortho(left, right, bottom, top, near, far);
                        mat4 view_matrix = look_at<double>(vec3(0, 0, 0), camera_direction, camera_up);

                        renderer->set_matrices(shadow_projection_matrix * shadow_view_matrix, projection_matrix * view_matrix);

                        renderer->set_light_direction(light_direction);
                        renderer->set_camera_direction(camera_direction);

                        vec4 screen_center((right + left) * 0.5, (top + bottom) * 0.5, (far + near) * 0.5, 1.0);
                        vec4 view_center = inverse(view_matrix) * screen_center;
                        m_camera.set_view_center_and_width(vec3(view_center[0], view_center[1], view_center[2]), right - left,
                                                           m_draw_width, m_draw_height);
                }

                if constexpr (API == GraphicsAndComputeAPI::Vulkan)
                {
                        if (!renderer->draw())
                        {
                                std::this_thread::sleep_until(last_frame_time + IDLE_MODE_FRAME_DURATION);
                                last_frame_time = std::chrono::steady_clock::now();
                        }
                }

                if constexpr (API == GraphicsAndComputeAPI::OpenGL)
                {
                        ASSERT(pencil_effect);
                        ASSERT(dft_show);
                        ASSERT(optical_flow);
                        ASSERT(convex_hull);
                        ASSERT(fps_text);

                        // Параметр true означает рисование в цветной буфер,
                        // параметр false означает рисование в буфер экрана.
                        // Если возвращает false, то нет объекта для рисования.
                        if (!renderer->draw(m_pencil_effect_active))
                        {
                                std::this_thread::sleep_until(last_frame_time + IDLE_MODE_FRAME_DURATION);
                                last_frame_time = std::chrono::steady_clock::now();
                        }

                        //

                        glViewport(0, 0, m_window_width, m_window_height);

                        if (m_pencil_effect_active)
                        {
                                // Рисование из цветного буфера в буфер экрана
                                pencil_effect->draw();
                        }

                        if (m_dft_show_active)
                        {
                                dft_show->take_image_from_framebuffer();
                        }
                        if (m_optical_flow_active)
                        {
                                optical_flow->take_image_from_framebuffer();
                        }

                        if (m_dft_show_active)
                        {
                                dft_show->draw();
                        }
                        if (m_optical_flow_active)
                        {
                                optical_flow->draw();
                        }
                        if (m_convex_hull_active)
                        {
                                convex_hull->draw();
                        }
                        if (m_fps_text_active)
                        {
                                fps_text->draw();
                        }

                        //

                        window->display();
                }
        }
}

template <GraphicsAndComputeAPI API>
void ShowObject<API>::loop_thread()
{
        ASSERT(std::this_thread::get_id() == m_thread.get_id());

        try
        {
                loop();
                if (!m_stop)
                {
                        m_callback->message_error_fatal("Thread ended.");
                }
        }
        catch (ErrorSourceException& e)
        {
                m_callback->message_error_source(e.msg(), e.src());
        }
        catch (std::exception& e)
        {
                m_callback->message_error_fatal(e.what());
        }
        catch (...)
        {
                m_callback->message_error_fatal("Unknown Error. Thread ended.");
        }
}
}

std::unique_ptr<IShow> create_show(GraphicsAndComputeAPI api, const ShowCreateInfo& info)
{
        switch (api)
        {
        case GraphicsAndComputeAPI::Vulkan:
                return std::make_unique<ShowObject<GraphicsAndComputeAPI::Vulkan>>(info);
        case GraphicsAndComputeAPI::OpenGL:
                return std::make_unique<ShowObject<GraphicsAndComputeAPI::OpenGL>>(info);
        }
        error_fatal("Unknown graphics and compute API for show creation");
}
