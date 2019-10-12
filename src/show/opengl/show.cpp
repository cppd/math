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

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "com/type/limit.h"
#include "gpu/convex_hull/opengl/show.h"
#include "gpu/dft/opengl/show.h"
#include "gpu/optical_flow/opengl/show.h"
#include "gpu/pencil_sketch/opengl/show.h"
#include "gpu/renderer/opengl/renderer.h"
#include "gpu/text/opengl/show.h"
#include "graphics/opengl/debug.h"
#include "graphics/opengl/functions.h"
#include "graphics/opengl/query.h"
#include "show/com/camera.h"
#include "show/com/event_queue.h"
#include "show/com/event_window.h"
#include "show/com/frame_rate.h"
#include "show/com/show_thread.h"
#include "window/manage.h"
#include "window/opengl/window.h"

constexpr double IDLE_MODE_FRAME_DURATION_IN_SECONDS = 0.1;

// Камера и тени рассчитаны на размер объекта 2 и на положение в точке (0, 0, 0).
constexpr double OBJECT_SIZE = 2;
constexpr vec3 OBJECT_POSITION = vec3(0);

//

constexpr int OPENGL_MINIMUM_SAMPLE_COUNT = 4;
constexpr GLenum OPENGL_FRAMEBUFFER_COLOR_FORMAT = GL_SRGB8;
constexpr GLenum OPENGL_FRAMEBUFFER_DEPTH_FORMAT = GL_DEPTH_COMPONENT32;
constexpr GLenum OPENGL_FRAMEBUFFER_RESOLVE_FORMAT = OPENGL_FRAMEBUFFER_COLOR_FORMAT;
constexpr GLenum OPENGL_OBJECT_IMAGE_FORMAT = GL_R32UI;

//

namespace
{
class Impl final : public Show, public WindowEvent
{
        static constexpr GLuint DEFAULT_FRAMEBUFFER = 0;

        EventQueue& m_event_queue;
        EventWindow<opengl::Window> m_event_window;
        ShowCallback* const m_callback;
        const WindowID m_parent_window;
        const double m_parent_window_ppi;
        const std::thread::id m_thread_id = std::this_thread::get_id();

        FrameRate m_frame_rate{m_parent_window_ppi};
        Camera m_camera;

        int m_draw_x = limits<int>::lowest();
        int m_draw_y = limits<int>::lowest();
        int m_draw_width = limits<int>::lowest();
        int m_draw_height = limits<int>::lowest();

        bool m_fullscreen_active = false;

        //

        Color m_text_color = Color(1);

        double m_dft_brightness = 1;
        Color m_dft_background_color = Color(0);
        Color m_dft_color = Color(1);

        bool m_text_active = true;
        bool m_pencil_sketch_active = false;
        bool m_dft_active = false;
        bool m_convex_hull_active = false;
        bool m_optical_flow_active = false;

        std::unique_ptr<opengl::Window> m_window;
        std::unique_ptr<opengl::DebugMessage> m_debug;
        std::unique_ptr<opengl::ColorDepthFramebufferMultisample> m_render_framebuffer;
        std::unique_ptr<opengl::ColorFramebuffer> m_resolve_framebuffer;
        std::unique_ptr<opengl::Texture> m_object_image;
        std::unique_ptr<gpu_opengl::Renderer> m_renderer;
        std::unique_ptr<gpu_opengl::Text> m_text;
        std::unique_ptr<gpu_opengl::DFTShow> m_dft;
        std::unique_ptr<gpu_opengl::ConvexHullShow> m_convex_hull;
        std::unique_ptr<gpu_opengl::OpticalFlowShow> m_optical_flow;
        std::unique_ptr<gpu_opengl::PencilSketchShow> m_pencil_sketch;

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
                m_text_color = background_is_dark ? Color(1) : Color(0);
                if (m_text)
                {
                        m_text->set_color(m_text_color);
                }
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

                m_text_active = v;
        }

        void show_pencil_sketch(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_pencil_sketch_active = v;
        }

        void show_dft(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_dft_active != v)
                {
                        m_dft_active = v;
                        window_resize_handler();
                }
        }

        void set_dft_brightness(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_dft_brightness = v;
                if (m_dft)
                {
                        m_dft->set_brightness(v);
                }
        }

        void set_dft_background_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_dft_background_color = c;
                if (m_dft)
                {
                        m_dft->set_background_color(c);
                }
        }

        void set_dft_color(const Color& c) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_dft_color = c;
                if (m_dft)
                {
                        m_dft->set_color(c);
                }
        }

        void show_convex_hull_2d(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_convex_hull_active = v;
                if (m_convex_hull)
                {
                        m_convex_hull->reset_timer();
                }
        }

        void show_optical_flow(bool v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_optical_flow_active = v;
                if (m_optical_flow)
                {
                        m_optical_flow->reset();
                }
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

                m_window->set_vertical_sync_enabled(v);
        }

        void set_shadow_zoom(double v) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_renderer->set_shadow_zoom(v);
        }

        ShowCameraInfo camera_information() const override
        {
                ASSERT(std::this_thread::get_id() != m_thread_id);

                return m_camera.show_info();
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
                if (right.pressed && right.pressed_x >= m_draw_x && right.pressed_x < m_draw_x + m_draw_width &&
                    right.pressed_y >= m_draw_y && right.pressed_y < m_draw_y + m_draw_height &&
                    (right.delta_x != 0 || right.delta_y != 0))
                {
                        m_camera.rotate(-right.delta_x, -right.delta_y);
                        changed = true;
                }

                const PressedMouseButton& left = m_event_window.pressed_mouse_button(MouseButton::Left);
                if (left.pressed && left.pressed_x >= m_draw_x && left.pressed_x < m_draw_x + m_draw_width &&
                    left.pressed_y >= m_draw_y && left.pressed_y < m_draw_y + m_draw_height &&
                    (left.delta_x != 0 || left.delta_y != 0))
                {
                        m_camera.move(vec2(-left.delta_x, left.delta_y));
                        changed = true;
                }

                if (changed)
                {
                        m_renderer->set_camera(m_camera.renderer_info());
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

                m_camera.scale(m_event_window.mouse_x() - m_draw_x, m_event_window.mouse_y() - m_draw_y, delta);

                m_renderer->set_camera(m_camera.renderer_info());
        }

        void reset_view_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.reset(vec3(1, 0, 0), vec3(0, 1, 0), 1, vec2(0, 0));

                m_renderer->set_camera(m_camera.renderer_info());
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

                resize();

                m_camera.resize(m_draw_width, m_draw_height);
                m_renderer->set_camera(m_camera.renderer_info());
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

                if (m_draw_x < 0 || m_draw_y < 0 || m_draw_width <= 0 || m_draw_height <= 0)
                {
                        error("Draw size error (" + to_string(m_draw_width) + ", " + to_string(m_draw_height) + ")");
                }

                reset_view_handler();
        }

        void resize()
        {
                m_draw_x = 0;
                m_draw_y = 0;
                m_draw_width = m_dft_active ? m_window->width() / 2 : m_window->width();
                m_draw_height = m_window->height();
                int dft_dst_x = m_window->width() - m_draw_width;
                int dft_dst_y = m_draw_y;

                m_convex_hull.reset();
                m_optical_flow.reset();
                m_dft.reset();
                m_pencil_sketch.reset();
                m_object_image.reset();
                m_resolve_framebuffer.reset();
                m_render_framebuffer.reset();

                m_render_framebuffer = std::make_unique<opengl::ColorDepthFramebufferMultisample>(
                        OPENGL_FRAMEBUFFER_COLOR_FORMAT, OPENGL_FRAMEBUFFER_DEPTH_FORMAT, OPENGL_MINIMUM_SAMPLE_COUNT,
                        m_window->width(), m_window->height());

                m_resolve_framebuffer = std::make_unique<opengl::ColorFramebuffer>(OPENGL_FRAMEBUFFER_RESOLVE_FORMAT,
                                                                                   m_draw_width, m_draw_height);

                m_object_image = std::make_unique<opengl::Texture>(OPENGL_OBJECT_IMAGE_FORMAT, m_draw_width, m_draw_height);

                //

                m_renderer->set_size(m_draw_x, m_draw_y, m_draw_width, m_draw_height, *m_object_image);

                //

                m_pencil_sketch = gpu_opengl::create_pencil_sketch_show(m_resolve_framebuffer->texture(), *m_object_image,
                                                                        m_draw_x, m_draw_y, m_draw_width, m_draw_height);

                m_dft = gpu_opengl::create_dft_show(m_resolve_framebuffer->texture(), m_draw_x, m_draw_y, dft_dst_x, dft_dst_y,
                                                    m_draw_width, m_draw_height, m_dft_brightness, m_dft_background_color,
                                                    m_dft_color);

                m_optical_flow = gpu_opengl::create_optical_flow_show(m_resolve_framebuffer->texture(), m_parent_window_ppi,
                                                                      m_draw_x, m_draw_y, m_draw_width, m_draw_height);

                m_convex_hull =
                        gpu_opengl::create_convex_hull_show(*m_object_image, m_draw_x, m_draw_y, m_draw_width, m_draw_height);

                if (!m_text)
                {
                        m_text = gpu_opengl::create_text(m_frame_rate.text_size(), m_text_color);
                }
                m_text->set_window(m_draw_x, m_draw_y, m_draw_width, m_draw_height);
        }

        void resolve_to_texture()
        {
                glBlitNamedFramebuffer(*m_render_framebuffer, *m_resolve_framebuffer, m_draw_x, m_draw_y, m_draw_x + m_draw_width,
                                       m_draw_y + m_draw_height, 0, 0, m_draw_width, m_draw_height, GL_COLOR_BUFFER_BIT,
                                       GL_NEAREST);
        }

        void resolve_to_default()
        {
                int width = m_event_window.window_width();
                int height = m_event_window.window_height();
                glBlitNamedFramebuffer(*m_render_framebuffer, DEFAULT_FRAMEBUFFER, 0, 0, width, height, 0, 0, width, height,
                                       GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        void render(const TextData& text_data)
        {
                ASSERT(m_render_framebuffer && m_resolve_framebuffer && m_object_image);
                ASSERT(m_pencil_sketch && m_dft && m_optical_flow && m_convex_hull && m_text);

                glBindFramebuffer(GL_FRAMEBUFFER, *m_render_framebuffer);

                m_renderer->draw();

                if (m_pencil_sketch_active)
                {
                        resolve_to_texture();
                        m_pencil_sketch->draw();
                }

                if (m_dft_active || m_optical_flow_active)
                {
                        resolve_to_texture();
                }

                if (m_dft_active)
                {
                        m_dft->draw();
                }
                if (m_optical_flow_active)
                {
                        m_optical_flow->draw();
                }
                if (m_convex_hull_active)
                {
                        m_convex_hull->draw();
                }
                if (m_text_active)
                {
                        m_text->draw(text_data);
                }

                resolve_to_default();

                m_window->display();
        }

public:
        Impl(EventQueue& event_queue, ShowCallback* callback, const WindowID& parent_window, double parent_window_ppi)
                : m_event_queue(event_queue),
                  m_callback(callback),
                  m_parent_window(parent_window),
                  m_parent_window_ppi(parent_window_ppi)
        {
                m_window = opengl::create_window();
                m_debug = opengl::create_debug_message();

                glDisable(GL_CULL_FACE);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_FRAMEBUFFER_SRGB);
                glEnable(GL_PROGRAM_POINT_SIZE);

                m_renderer = gpu_opengl::create_renderer();

                m_event_window.set_window(*m_window);

                init_window_and_view();
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void loop(std::atomic_bool& stop)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                double last_frame_time = time_in_seconds();
                while (!stop)
                {
                        pull_and_dispatch_all_events();

                        m_frame_rate.calculate();

                        render(m_frame_rate.text_data());

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

namespace show_opengl
{
std::unique_ptr<ShowObject> create_show_object(const ShowCreateInfo& info)
{
        return std::make_unique<ShowThread<Impl>>(info);
}
}
