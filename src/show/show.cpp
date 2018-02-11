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

#include "event.h"
#include "color_space/buffer_type.h"
#include "renderer/renderer.h"
#include "text/text.h"

#include "com/color/colors.h"
#include "com/error.h"
#include "com/log.h"
#include "com/mat.h"
#include "com/mat_alg.h"
#include "com/math.h"
#include "com/print.h"
#include "com/quaternion.h"
#include "com/thread.h"
#include "com/time.h"
#include "gpu_2d/convex_hull/convex_hull_2d.h"
#include "gpu_2d/dft/show/dft_show.h"
#include "gpu_2d/optical_flow/optical_flow.h"
#include "gpu_2d/pencil/pencil.h"
#include "graphics/objects.h"
#include "numerical/linear.h"
#include "obj/obj.h"
#include "window/window.h"

#include <SFML/Window/Event.hpp>
#include <cmath>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr double ZOOM_BASE = 1.1;
constexpr double ZOOM_EXP_MIN = -50;
constexpr double ZOOM_EXP_MAX = 100;

constexpr double PI_DIV_180 = PI<double> / 180;
constexpr double to_radians(double angle)
{
        return angle * PI_DIV_180;
}

namespace
{
long get_fps(double* start)
{
        double now = time_in_seconds();
        double time_elapsed = now - *start;
        *start = now;
        return (time_elapsed > 0) ? lround(1.0 / time_elapsed) : 0;
}

vec3 rotate_vector_degree(const vec3& axis, double angle_degree, const vec3& v)
{
        return rotate_vector(axis, to_radians(angle_degree), v);
}

#if 0
int get_object_under_mouse(int mouse_x, int mouse_y, int window_height, const TextureR32I& tex)
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

class Camera final
{
        mutable SpinLock m_lock;

        vec3 m_camera_right, m_camera_up, m_camera_direction, m_light_up, m_light_direction;

        vec3 m_view_center;
        double m_view_width;

        int m_paint_width = -1;
        int m_paint_height = -1;

        void set_vectors(const vec3& right, const vec3& up)
        {
                m_camera_up = normalize(up);

                // от поверхности на камеру
                m_camera_direction = cross(normalize(right), m_camera_up);

                m_camera_right = cross(m_camera_up, m_camera_direction);

                vec3 light_right = rotate_vector_degree(m_camera_up, -45, m_camera_right);
                m_light_up = rotate_vector_degree(light_right, -45, m_camera_up);

                // от поверхности на свет
                m_light_direction = cross(light_right, m_light_up);
        }

public:
        Camera() : m_camera_right(0), m_camera_up(0), m_camera_direction(0), m_light_up(0), m_light_direction(0)
        {
        }

        void set(const vec3& right, const vec3& up)
        {
                std::lock_guard lg(m_lock);

                set_vectors(right, up);
        }

        void get(vec3* camera_up, vec3* camera_direction, vec3* light_up, vec3* light_direction) const
        {
                std::lock_guard lg(m_lock);

                *camera_up = m_camera_up;
                *camera_direction = -m_camera_direction; // от камеры на объект
                *light_up = m_light_up;
                *light_direction = -m_light_direction; // от источника света на объект
        }

        void get_camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width) const
        {
                std::lock_guard lg(m_lock);

                *camera_up = m_camera_up;
                *camera_direction = -m_camera_direction; // от камеры на объект
                *view_center = m_view_center;
                *view_width = m_view_width;
        }

        void get_paint_width_height(int* width, int* height) const
        {
                std::lock_guard lg(m_lock);

                *width = m_paint_width;
                *height = m_paint_height;
        }

        vec3 get_light_direction() const
        {
                std::lock_guard lg(m_lock);

                return -m_light_direction; // от источника света на объект
        }

        void rotate(int delta_x, int delta_y)
        {
                std::lock_guard lg(m_lock);

                vec3 right = rotate_vector_degree(m_camera_up, -delta_x, m_camera_right);
                vec3 up = rotate_vector_degree(m_camera_right, -delta_y, m_camera_up);
                set_vectors(right, up);
        }

        void set_view_center_and_width(const vec3& vec, double view_width, int paint_width, int paint_height)
        {
                std::lock_guard lg(m_lock);

                m_view_center = vec;
                m_view_width = view_width;

                m_paint_width = paint_width;
                m_paint_height = paint_height;
        }
};

class ShowObject final : public IShow
{
        IShowCallback* const m_callback;
        const WindowID m_win_parent;
        std::thread m_thread;
        ThreadQueue<Event> m_event_queue;
        std::atomic_bool m_stop{false};

        Camera m_camera;

        // Камера и тени рассчитаны на размер объекта 2 и на положение в точке (0, 0, 0).
        static constexpr double OBJECT_SIZE = 2;
        static constexpr vec3 OBJECT_POSITION = vec3(0);

        void loop();
        void loop_thread();

        void add_object(const std::shared_ptr<const IObj>& obj_ptr, int id, int scale_id) override
        {
                m_event_queue.emplace(std::in_place_type<Event::add_object>, obj_ptr, id, scale_id);
        }
        void delete_object(int id) override
        {
                m_event_queue.emplace(std::in_place_type<Event::delete_object>, id);
        }
        void show_object(int id) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_object>, id);
        }
        virtual void delete_all_objects() override
        {
                m_event_queue.emplace(std::in_place_type<Event::delete_all_objects>);
        }
        void reset_view() override
        {
                m_event_queue.emplace(std::in_place_type<Event::reset_view>);
        }
        void set_ambient(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_ambient>, v);
        }
        void set_diffuse(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_diffuse>, v);
        }
        void set_specular(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_specular>, v);
        }
        void set_background_color_rgb(const vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_background_color>, c);
        }
        void set_default_color_rgb(const vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_color>, c);
        }
        void set_wireframe_color_rgb(const vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_wireframe_color>, c);
        }
        void set_default_ns(double ns) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_ns>, ns);
        }
        void show_smooth(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_smooth>, v);
        }
        void show_wireframe(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_wireframe>, v);
        }
        void show_shadow(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_shadow>, v);
        }
        void show_materials(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_materials>, v);
        }
        void show_effect(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_effect>, v);
        }
        void show_dft(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_dft>, v);
        }
        void set_dft_brightness(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_brightness>, v);
        }
        void show_convex_hull_2d(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_convex_hull_2d>, v);
        }
        void show_optical_flow(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_optical_flow>, v);
        }
        void parent_resized() override
        {
                m_event_queue.emplace(std::in_place_type<Event::parent_resized>);
        }
        void mouse_wheel(double delta) override
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_wheel>, delta);
        }
        void toggle_fullscreen() override
        {
                m_event_queue.emplace(std::in_place_type<Event::toggle_fullscreen>);
        }
        void set_vertical_sync(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::vertical_sync>, v);
        }
        void set_shadow_zoom(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::shadow_zoom>, v);
        }

        void get_camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width) const override
        {
                m_camera.get_camera_information(camera_up, camera_direction, view_center, view_width);
        }
        void get_paint_width_height(int* width, int* height) const override
        {
                m_camera.get_paint_width_height(width, height);
        }
        vec3 get_light_direction() const override
        {
                return m_camera.get_light_direction();
        }
        double get_object_size() const override
        {
                return OBJECT_SIZE;
        }
        vec3 get_object_position() const override
        {
                return OBJECT_POSITION;
        }

public:
        ShowObject(IShowCallback* callback, WindowID win_parent, vec3 background_color_rgb, vec3 default_color_rgb,
                   vec3 wireframe_color_rgb, bool with_smooth, bool with_wireframe, bool with_shadow, bool with_materials,
                   bool with_effect, bool with_dft, bool with_convex_hull, bool with_optical_flow, double ambient, double diffuse,
                   double specular, double dft_brightness, double default_ns, bool vertical_sync, double shadow_zoom)
                : m_callback(callback), m_win_parent(win_parent)

        {
                if (!callback)
                {
                        error("No callback pointer");
                }

                reset_view();
                set_ambient(ambient);
                set_diffuse(diffuse);
                set_specular(specular);
                set_background_color_rgb(background_color_rgb);
                set_default_color_rgb(default_color_rgb);
                set_wireframe_color_rgb(wireframe_color_rgb);
                set_default_ns(default_ns);
                show_smooth(with_smooth);
                show_wireframe(with_wireframe);
                show_shadow(with_shadow);
                show_effect(with_effect);
                show_dft(with_dft);
                set_dft_brightness(dft_brightness);
                show_materials(with_materials);
                show_convex_hull_2d(with_convex_hull);
                show_optical_flow(with_optical_flow);
                set_vertical_sync(vertical_sync);
                set_shadow_zoom(shadow_zoom);

                m_thread = std::thread(&ShowObject::loop_thread, this);
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

void ShowObject::loop()
{
        {
                // Без этого создания контекста почему-то не сможет установиться
                // ANTIALIASING_LEVEL в ненулевое значение далее при создании окна.
                // В версии SFML 2.4.2 эта проблема исчезла.
                std::unique_ptr<sf::Context> context;
                create_gl_context_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions(), &context);
        }

        sf::Window wnd;
        create_gl_window_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions(), ANTIALIASING_LEVEL, DEPTH_BITS,
                             STENCIL_BITS, RED_BITS, GREEN_BITS, BLUE_BITS, ALPHA_BITS, &wnd);
        move_window_to_parent(wnd.getSystemHandle(), m_win_parent);

        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_FRAMEBUFFER_SRGB);

        const bool framebuffer_srgb = frame_buffer_is_srgb();
        const bool colorbuffer_srgb = color_buffer_is_srgb();

        LOG(framebuffer_srgb ? "Framebuffer sRGB" : "Framebuffer linear");
        LOG(colorbuffer_srgb ? "Colorbuffer sRGB" : "Colorbuffer linear");

        int new_width = wnd.getSize().x;
        int new_height = wnd.getSize().y;
        double pixel_to_coord_no_zoom = 2.0 / std::min(new_width, new_height);
        double pixel_to_coord = pixel_to_coord_no_zoom;

        // обязательно задать начальные значения -1, чтобы отработала функция изменения размеров окна
        int width = -1, height = -1, window_width = -1, window_height = -1;

        int mouse_x = 0, mouse_y = 0, new_mouse_x = 0, new_mouse_y = 0;
        bool mouse_pressed = false, mouse_pressed_shift = false;
        vec2 window_center(0, 0);
        double zoom_delta = 0;
        double wheel_delta = 0;

        // Неважно, какие тут будут значения при инициализации,
        // так как при создании окна начальные параметры помещаются в очередь
        bool dft_active = false;
        bool dft_active_old = false;
        double dft_brightness = -1;
        bool pencil_effect_active = false;
        bool convex_hull_2d_active = false;
        bool optical_flow_active = false;
        bool default_view = false;

        // Вначале без полноэкранного режима
        bool fullscreen_active = false;

        std::unique_ptr<IRenderer> renderer = create_renderer();

        std::unique_ptr<DFTShow> dft_show;
        std::unique_ptr<PencilEffect> pencil_effect;
        std::unique_ptr<OpticalFlow> optical_flow;
        std::unique_ptr<ConvexHull2D> convex_hull_2d;

        Text text;

        double start_time = time_in_seconds();

        while (true)
        {
                if (m_stop)
                {
#if defined(_WIN32)
                        // Без этого вызова почему-то зависает деструктор окна SFML на Винде,
                        // если это окно встроено в родительское окно.
                        change_window_style_not_child(wnd.getSystemHandle());
#endif

                        return;
                }

                while (true)
                {
                        std::optional<Event> event(m_event_queue.pop());

                        if (!event)
                        {
                                break;
                        }

                        switch (event->get_type())
                        {
                        case Event::EventType::add_object:
                        {
                                const Event::add_object& d = event->get<Event::add_object>();

                                if (!d.obj)
                                {
                                        error("Null object received");
                                }

                                renderer->add_object(d.obj.get(), OBJECT_SIZE, OBJECT_POSITION, d.id, d.scale_id);
                                m_callback->object_loaded(d.id);
                                break;
                        }
                        case Event::EventType::delete_object:
                        {
                                const Event::delete_object& d = event->get<Event::delete_object>();

                                renderer->delete_object(d.id);
                                break;
                        }
                        case Event::EventType::show_object:
                        {
                                const Event::show_object& d = event->get<Event::show_object>();

                                renderer->show_object(d.id);
                                break;
                        }
                        case Event::EventType::delete_all_objects:
                        {
                                renderer->delete_all();
                                default_view = true;
                                break;
                        }
                        case Event::EventType::parent_resized:
                        {
                                if (!fullscreen_active)
                                {
                                        set_size_to_parent(wnd.getSystemHandle(), m_win_parent);
                                }
                                break;
                        }
                        case Event::EventType::toggle_fullscreen:
                        {
                                fullscreen_active = !fullscreen_active;
                                make_fullscreen(fullscreen_active, wnd.getSystemHandle(), m_win_parent);
                                break;
                        }
                        case Event::EventType::reset_view:
                        {
                                default_view = true;
                                break;
                        }
                        case Event::EventType::mouse_wheel:
                        {
                                // Для полноэкранного режима обрабатывается
                                // в другом сообщении sf::Event::MouseWheelScrolled
                                if (!fullscreen_active)
                                {
                                        const Event::mouse_wheel& d = event->get<Event::mouse_wheel>();
                                        if (new_mouse_x < width && new_mouse_y < height)
                                        {
                                                wheel_delta = d.delta;
                                        }
                                }
                                break;
                        }
                        case Event::EventType::set_ambient:
                        {
                                const Event::set_ambient& d = event->get<Event::set_ambient>();

                                vec3 light = vec3(d.ambient);
                                renderer->set_light_a(light);
                                break;
                        }
                        case Event::EventType::set_diffuse:
                        {
                                const Event::set_diffuse& d = event->get<Event::set_diffuse>();

                                vec3 light = vec3(d.diffuse);
                                renderer->set_light_d(light);
                                break;
                        }
                        case Event::EventType::set_specular:
                        {
                                const Event::set_specular& d = event->get<Event::set_specular>();

                                vec3 light = vec3(d.specular);
                                renderer->set_light_s(light);
                                break;
                        }
                        case Event::EventType::set_background_color:
                        {
                                const Event::set_background_color& d = event->get<Event::set_background_color>();

                                vec3 color = d.background_color;
                                glClearColor(color[0], color[1], color[2], 1);
                                bool dark_color = luminance_of_rgb(color) <= 0.5;
                                text.set_color(dark_color ? vec3(1) : vec3(0));
                                break;
                        }
                        case Event::EventType::set_default_color:
                        {
                                const Event::set_default_color& d = event->get<Event::set_default_color>();

                                vec3 color = d.default_color;
                                renderer->set_default_color(color);
                                break;
                        }
                        case Event::EventType::set_wireframe_color:
                        {
                                const Event::set_wireframe_color& d = event->get<Event::set_wireframe_color>();

                                vec3 color = d.wireframe_color;
                                renderer->set_wireframe_color(color);
                                break;
                        }
                        case Event::EventType::set_default_ns:
                        {
                                const Event::set_default_ns& d = event->get<Event::set_default_ns>();

                                renderer->set_default_ns(d.default_ns);
                                break;
                        }
                        case Event::EventType::show_smooth:
                        {
                                const Event::show_smooth& d = event->get<Event::show_smooth>();

                                renderer->set_show_smooth(d.show);
                                break;
                        }
                        case Event::EventType::show_wireframe:
                        {
                                const Event::show_wireframe& d = event->get<Event::show_wireframe>();

                                renderer->set_show_wireframe(d.show);
                                break;
                        }
                        case Event::EventType::show_shadow:
                        {
                                const Event::show_shadow& d = event->get<Event::show_shadow>();

                                renderer->set_show_shadow(d.show);
                                break;
                        }
                        case Event::EventType::show_materials:
                        {
                                const Event::show_materials& d = event->get<Event::show_materials>();

                                renderer->set_show_materials(d.show);
                                break;
                        }
                        case Event::EventType::show_effect:
                        {
                                const Event::show_effect& d = event->get<Event::show_effect>();

                                pencil_effect_active = d.show;
                                break;
                        }
                        case Event::EventType::show_dft:
                        {
                                const Event::show_dft& d = event->get<Event::show_dft>();

                                dft_active = d.show;
                                break;
                        }
                        case Event::EventType::set_dft_brightness:
                        {
                                const Event::set_dft_brightness& d = event->get<Event::set_dft_brightness>();

                                dft_brightness = d.dft_brightness;
                                if (dft_show)
                                {
                                        dft_show->set_brightness(d.dft_brightness);
                                }
                                break;
                        }
                        case Event::EventType::show_convex_hull_2d:
                        {
                                const Event::show_convex_hull_2d& d = event->get<Event::show_convex_hull_2d>();

                                convex_hull_2d_active = d.show;
                                if (convex_hull_2d)
                                {
                                        convex_hull_2d->reset_timer();
                                }
                                break;
                        }
                        case Event::EventType::show_optical_flow:
                        {
                                const Event::show_optical_flow& d = event->get<Event::show_optical_flow>();

                                optical_flow_active = d.show;
                                if (optical_flow)
                                {
                                        optical_flow->reset();
                                }
                                break;
                        }
                        case Event::EventType::vertical_sync:
                        {
                                const Event::vertical_sync& d = event->get<Event::vertical_sync>();

                                wnd.setVerticalSyncEnabled(d.enable);
                                break;
                        }
                        case Event::EventType::shadow_zoom:
                        {
                                const Event::shadow_zoom& d = event->get<Event::shadow_zoom>();

                                renderer->set_shadow_zoom(d.zoom);
                                break;
                        }
                        }
                }

                while (true)
                {
                        sf::Event event;

                        if (!wnd.pollEvent(event))
                        {
                                break;
                        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
                        switch (event.type)
                        {
                        case sf::Event::Closed:
                                break;
                        case sf::Event::KeyPressed:
                                switch (event.key.code)
#pragma GCC diagnostic pop
                                {
                                case sf::Keyboard::F11:
                                        toggle_fullscreen();
                                        break;
                                case sf::Keyboard::Escape:
                                        if (fullscreen_active)
                                        {
                                                toggle_fullscreen();
                                        }
                                        break;
                                }
                                break;
                        case sf::Event::MouseButtonPressed:
                                if (event.mouseButton.x < width && event.mouseButton.y < height &&
                                    (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Right))
                                {
                                        mouse_pressed = true;
                                        mouse_pressed_shift = (event.mouseButton.button == sf::Mouse::Left);
                                        mouse_x = event.mouseButton.x;
                                        mouse_y = event.mouseButton.y;
                                }
                                break;
                        case sf::Event::MouseButtonReleased:
                                if (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Right)
                                {
                                        mouse_pressed = false;
                                }
                                break;
                        case sf::Event::MouseMoved:
                                new_mouse_x = event.mouseMove.x;
                                new_mouse_y = event.mouseMove.y;
                                break;
                        case sf::Event::MouseWheelScrolled:
                                // Для режима встроенного окна перенесено на обработчик события
                                // Event::EventType::mouse_wheel, так как на Винде не приходит
                                // это сообщение для дочернего окна.
                                if (fullscreen_active)
                                {
                                        // if (new_mouse_x < width && new_mouse_y < height &&
                                        //    get_object_under_mouse(new_mouse_x, new_mouse_y, window_height, *object_texture) >
                                        //    0)
                                        if (new_mouse_x < width && new_mouse_y < height)
                                        {
                                                wheel_delta = event.mouseWheelScroll.delta;
                                        }
                                }
                                break;
                        case sf::Event::Resized:
                                new_width = event.size.width;
                                new_height = event.size.height;
                                break;
                        }
                }

                bool matrix_change = false;

                if (mouse_pressed && (new_mouse_x != mouse_x || new_mouse_y != mouse_y))
                {
                        int delta_x = new_mouse_x - mouse_x;
                        int delta_y = new_mouse_y - mouse_y;
                        mouse_x = new_mouse_x;
                        mouse_y = new_mouse_y;

                        if (!mouse_pressed_shift)
                        {
                                m_camera.rotate(delta_x, delta_y);
                        }
                        else
                        {
                                window_center -= pixel_to_coord * vec2(delta_x, -delta_y);
                        }

                        matrix_change = true;
                }

                if (wheel_delta != 0)
                {
                        if ((wheel_delta < 0 && zoom_delta > ZOOM_EXP_MIN) || (wheel_delta > 0 && zoom_delta < ZOOM_EXP_MAX))
                        {
                                zoom_delta += wheel_delta;

                                vec2 mouse_in_wnd(new_mouse_x - width * 0.5, height * 0.5 - new_mouse_y);

                                window_center +=
                                        pixel_to_coord * (mouse_in_wnd - mouse_in_wnd * std::pow(ZOOM_BASE, -wheel_delta));

                                pixel_to_coord = pixel_to_coord_no_zoom * std::pow(ZOOM_BASE, -zoom_delta);

                                matrix_change = true;
                        }
                        wheel_delta = 0;
                }

                if (window_width != new_width || window_height != new_height || dft_active_old != dft_active)
                {
                        // Чтобы в случае исключений не остались объекты от прошлых размеров окна,
                        // вначале нужно удалить все объекты.

                        renderer->free_buffers();

                        dft_show = nullptr;
                        pencil_effect = nullptr;
                        optical_flow = nullptr;
                        convex_hull_2d = nullptr;

                        window_width = new_width;
                        window_height = new_height;

                        width = dft_active ? window_width / 2 : window_width;
                        height = window_height;

                        dft_active_old = dft_active;

                        matrix_change = true;

                        // матрица для рисования на плоскости, 0 вверху
                        mat4 plane_matrix = scale<double>(2.0 / window_width, -2.0 / window_height, 1) *
                                            translate<double>(-window_width / 2.0, -window_height / 2.0, 0);

                        renderer->set_size(width, height);

                        int dft_pos_x = (window_width & 1) ? (width + 1) : width;
                        int dft_pos_y = 0;
                        dft_show = std::make_unique<DFTShow>(width, height, dft_pos_x, dft_pos_y, plane_matrix, framebuffer_srgb);
                        dft_show->set_brightness(dft_brightness);

                        pencil_effect = std::make_unique<PencilEffect>(renderer->get_color_buffer_texture(),
                                                                       renderer->get_object_texture(), colorbuffer_srgb);

                        optical_flow = std::make_unique<OpticalFlow>(width, height, plane_matrix);

                        convex_hull_2d = std::make_unique<ConvexHull2D>(renderer->get_object_texture(), plane_matrix);
                }

                if (default_view)
                {
                        default_view = false;

                        zoom_delta = 0;
                        window_center = vec2(0, 0);
                        pixel_to_coord_no_zoom = 2.0 / std::min(width, height);
                        pixel_to_coord = pixel_to_coord_no_zoom;
                        m_camera.set(vec3(1, 0, 0), vec3(0, 1, 0));

                        matrix_change = true;
                }

                if (matrix_change)
                {
                        vec3 camera_up, camera_direction, light_up, light_direction;

                        m_camera.get(&camera_up, &camera_direction, &light_up, &light_direction);

                        mat4 shadow_matrix =
                                ortho<double>(-1, 1, -1, 1, -1, 1) * look_at(vec3(0, 0, 0), light_direction, light_up);

                        double left = -0.5 * width * pixel_to_coord;
                        double right = 0.5 * width * pixel_to_coord;
                        double bottom = -0.5 * height * pixel_to_coord;
                        double top = 0.5 * height * pixel_to_coord;
                        double z_near = -1.0;
                        double z_far = 1.0;

                        mat4 projection_matrix = ortho<double>(left, right, bottom, top, z_near, z_far);

                        mat4 view_matrix = translate<double>(-window_center[0], -window_center[1], 0) *
                                           look_at<double>(vec3(0, 0, 0), camera_direction, camera_up);

                        renderer->set_matrices(shadow_matrix, projection_matrix * view_matrix);

                        renderer->set_light_direction(-light_direction);
                        renderer->set_camera_direction(-camera_direction);

                        vec4 screen_center((right + left) * 0.5, (top + bottom) * 0.5, (z_far + z_near) * 0.5, 1.0);
                        vec4 view_center = inverse(view_matrix) * screen_center;
                        m_camera.set_view_center_and_width(vec3(view_center[0], view_center[1], view_center[2]), right - left,
                                                           width, height);
                }

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                //

                glEnable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);

                // Если pencil_effect_active, то рисование в цветной буфер
                renderer->draw(pencil_effect_active);

                //

                // Рисование из цветного буфера в буфер экрана
                if (pencil_effect_active && pencil_effect)
                {
                        pencil_effect->draw();
                }

                if (dft_active && dft_show)
                {
                        dft_show->copy_image();
                }
                if (optical_flow_active && optical_flow)
                {
                        optical_flow->copy_image();
                }

                //

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);

                glViewport(0, 0, window_width, window_height);

                if (dft_active && dft_show)
                {
                        dft_show->draw();
                }

                glEnable(GL_SCISSOR_TEST);
                glScissor(0, 0, width, height);
                if (optical_flow_active && optical_flow)
                {
                        optical_flow->draw();
                }
                if (convex_hull_2d_active && convex_hull_2d)
                {
                        convex_hull_2d->draw();
                }
                glDisable(GL_SCISSOR_TEST);

                //

                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                text.draw(window_width, window_height, {"FPS: " + std::to_string(get_fps(&start_time))});

                wnd.display();
        }
}

void ShowObject::loop_thread()
{
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
                m_callback->message_error_source(e.get_msg(), e.get_src());
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

std::unique_ptr<IShow> create_show(IShowCallback* cb, WindowID win_parent, vec3 background_color_rgb, vec3 default_color_rgb,
                                   vec3 wireframe_color_rgb, bool with_smooth, bool with_wireframe, bool with_shadow,
                                   bool with_materials, bool with_effect, bool with_dft, bool with_convex_hull,
                                   bool with_optical_flow, double ambient, double diffuse, double specular, double dft_brightness,
                                   double default_ns, bool vertical_sync, double shadow_zoom)
{
        return std::make_unique<ShowObject>(cb, win_parent, background_color_rgb, default_color_rgb, wireframe_color_rgb,
                                            with_smooth, with_wireframe, with_shadow, with_materials, with_effect, with_dft,
                                            with_convex_hull, with_optical_flow, ambient, diffuse, specular, dft_brightness,
                                            default_ns, vertical_sync, shadow_zoom);
}
