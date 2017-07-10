/*
Copyright (C) 2017 Topological Manifold

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

#include "draw_object.h"
#include "show_event.h"

#include "color/color_space.h"
#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/print_glm.h"
#include "com/thread.h"
#include "com/time.h"
#include "dft_show/dft_show.h"
#include "gl/gl_objects.h"
#include "hull_2d/hull_2d.h"
#include "obj/obj.h"
#include "optical_flow/optical_flow.h"
#include "pencil/pencil.h"
#include "text/text.h"
#include "window/window.h"

#include <SFML/Window/Event.hpp>
#include <cmath>
#include <glm/gtc/color_space.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr float ZOOM_BASE = 1.1;
constexpr float ZOOM_EXP_MIN = -50;
constexpr float ZOOM_EXP_MAX = 100;

constexpr float PI_DIV_180 = PI / 180;
constexpr float to_radians(float angle)
{
        return angle * PI_DIV_180;
}

namespace
{
long get_fps(double* start)
{
        double now = get_time_seconds();
        double time_elapsed = now - *start;
        *start = now;
        return (time_elapsed > 0) ? lround(1.0 / time_elapsed) : 0;
}

#if 0
int get_object_under_mouse(int mouse_x, int mouse_y, int window_height, const TextureR32I& tex)
{
        int x = mouse_x;
        int y = window_height - mouse_y - 1;
        GLint v;
        tex.get_texture_sub_image(x, y, 1, 1, &v);
        return v;
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
        glm::vec3 m_camera_right, m_camera_up, m_camera_direction, m_light_up, m_light_direction;

        void set_vectors(const glm::vec3& right, const glm::vec3& up)
        {
                m_camera_up = glm::normalize(up);

                // от поверхности на камеру
                m_camera_direction = glm::cross(glm::normalize(right), m_camera_up);

                m_camera_right = glm::cross(m_camera_up, m_camera_direction);

                glm::vec3 light_right = glm::rotate(m_camera_right, to_radians(-45), m_camera_up);
                m_light_up = glm::rotate(m_camera_up, to_radians(-45), light_right);

                // от поверхности на свет
                m_light_direction = glm::cross(light_right, m_light_up);
        }

public:
        Camera() : m_camera_right(0), m_camera_up(0), m_camera_direction(0), m_light_up(0), m_light_direction(0)
        {
        }
        void set(const glm::vec3& right, const glm::vec3& up)
        {
                set_vectors(right, up);
        }
        void rotate(int delta_x, int delta_y)
        {
                glm::vec3 right = glm::rotate(m_camera_right, to_radians(-delta_x), m_camera_up);
                glm::vec3 up = glm::rotate(m_camera_up, to_radians(-delta_y), m_camera_right);
                set_vectors(right, up);
        }
        // glm::vec3 right() const
        //{
        //        return m_camera_right;
        //}
        const glm::vec3& up() const
        {
                return m_camera_up;
        }
        const glm::vec3& dir() const
        {
                return m_camera_direction;
        }
        const glm::vec3& light_up() const
        {
                return m_light_up;
        }
        const glm::vec3& light_dir() const
        {
                return m_light_direction;
        }
};

class DrawObjects final
{
        struct MapEntry
        {
                std::unique_ptr<IDrawObject> object;
                int scale_object_id;
                MapEntry(std::unique_ptr<IDrawObject>&& obj_, int scale_id_) : object(std::move(obj_)), scale_object_id(scale_id_)
                {
                }
        };

        std::unordered_map<int, MapEntry> m_objects;

        const IDrawObject* m_draw_object = nullptr;
        const IDrawObject* m_draw_scale_object = nullptr;
        int m_draw_scale_object_id = 0;

public:
        void add_object(std::unique_ptr<IDrawObject>&& object, int id, int scale_id)
        {
                if (id == m_draw_scale_object_id)
                {
                        m_draw_scale_object = object.get();
                }

                m_objects.insert_or_assign(id, MapEntry(std::move(object), scale_id));
        }

        void delete_object(int id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        if (iter->second.object.get() == m_draw_object)
                        {
                                m_draw_object = nullptr;
                        }
                        if (iter->second.object.get() == m_draw_scale_object)
                        {
                                m_draw_scale_object = nullptr;
                        }
                        m_objects.erase(iter);
                }
        }

        void show_object(int id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        m_draw_object = iter->second.object.get();

                        m_draw_scale_object_id = iter->second.scale_object_id;

                        auto scale_iter = m_objects.find(m_draw_scale_object_id);
                        if (scale_iter != m_objects.cend())
                        {
                                m_draw_scale_object = scale_iter->second.object.get();
                        }
                        else
                        {
                                m_draw_scale_object = nullptr;
                        }
                }
                else
                {
                        m_draw_object = nullptr;
                }
        }

        void delete_all()
        {
                m_objects.clear();
                m_draw_object = nullptr;
        }

        const IDrawObject* get_object() const
        {
                return m_draw_object;
        }

        const IDrawObject* get_scale_object() const
        {
                return m_draw_scale_object;
        }
};

class ShowObject final : public IShow
{
        IShowCallback* const m_callback;
        const WindowID m_win_parent;
        std::thread m_thread;
        ThreadQueue<Event> m_event_queue;
        std::atomic_bool m_stop{false};

        void loop();
        void loop_thread();

        void add_object(const std::shared_ptr<IObj>& obj_ptr, int id, int scale_id) override
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
        void set_ambient(float v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_ambient>, v);
        }
        void set_diffuse(float v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_diffuse>, v);
        }
        void set_specular(float v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_specular>, v);
        }
        void set_clear_color(const glm::vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_clear_color>, c);
        }
        void set_default_color(const glm::vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_color>, c);
        }
        void set_wireframe_color(const glm::vec3& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_wireframe_color>, c);
        }
        void set_default_ns(float ns) override
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
        void set_dft_brightness(float v) override
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

public:
        ShowObject(IShowCallback* callback, WindowID win_parent, glm::vec3 clear_color, glm::vec3 default_color,
                   glm::vec3 wireframe_color, bool with_smooth, bool with_wireframe, bool with_shadow, bool with_materials,
                   bool with_effect, bool with_dft, bool with_convex_hull, bool with_optical_flow, float ambient, float diffuse,
                   float specular, float dft_brightness, float default_ns)
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
                set_clear_color(clear_color);
                set_default_color(default_color);
                set_wireframe_color(wireframe_color);
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

        // get_framebuffer_sRGB() (glGetNamedFramebufferAttachmentParameteriv) возвращает
        // неправильные значения. Текстура передаётся из экранного буфера
        // и, вероятно, имеет тип sRGB, поэтому надо преобразовывать обратно в RGB.
        constexpr bool buffer_sRGB = true; // true вместо get_framebuffer_sRGB()

        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_FRAMEBUFFER_SRGB);

        int new_width = wnd.getSize().x;
        int new_height = wnd.getSize().y;
        float pixel_to_coord_no_zoom = 2.0f / std::min(new_width, new_height);
        float pixel_to_coord = pixel_to_coord_no_zoom;

        // обязательно задать начальные значения -1, чтобы отработала функция изменения размеров окна
        int width = -1, height = -1, window_width = -1, window_height = -1;

        int mouse_x = 0, mouse_y = 0, new_mouse_x = 0, new_mouse_y = 0;
        bool mouse_pressed = false, mouse_pressed_shift = false;
        glm::vec2 window_center(0, 0);
        float zoom_delta = 0;
        float wheel_delta = 0;

        // Неважно, какие тут будут значения при инициализации,
        // так как при создании окна начальные параметры помещаются в очередь
        bool dft_active = false;
        bool dft_active_old = false;
        float dft_brightness = -1.0f;
        bool shadow_active = false;
        bool pencil_effect_active = false;
        bool convex_hull_2d_active = false;
        bool optical_flow_active = false;
        bool default_view = false;

        // Вначале без полноэкранного режима
        bool fullscreen_active = false;

        std::unique_ptr<IDrawProgram> draw_program = create_draw_program();

        std::unique_ptr<DFTShow> dft_show;
        std::unique_ptr<PencilEffect> pencil_effect;
        std::unique_ptr<OpticalFlow> optical_flow;
        std::unique_ptr<ConvexHull2D> convex_hull_2d;

        ColorSpaceConverterToRGB color_converter;
        Text text;
        Camera camera;
        DrawObjects objects;

        Event event;

        double start_time = get_time_seconds();

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
                        if (!m_event_queue.pop(event))
                        {
                                break;
                        }

                        switch (event.get_type())
                        {
                        case Event::EventType::add_object:
                        {
                                const Event::add_object& d = event.get<Event::add_object>();
                                if (!d.obj)
                                {
                                        error("Null object received");
                                }

                                objects.add_object(create_draw_object(d.obj.get(), color_converter), d.id, d.scale_id);

                                m_callback->object_loaded(d.id);

                                break;
                        }
                        case Event::EventType::delete_object:
                        {
                                const Event::delete_object& d = event.get<Event::delete_object>();

                                objects.delete_object(d.id);

                                break;
                        }
                        case Event::EventType::show_object:
                        {
                                const Event::show_object& d = event.get<Event::show_object>();

                                objects.show_object(d.id);

                                break;
                        }
                        case Event::EventType::delete_all_objects:
                        {
                                objects.delete_all();

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
                                        const Event::mouse_wheel& d = event.get<Event::mouse_wheel>();
                                        if (new_mouse_x < width && new_mouse_y < height)
                                        {
                                                wheel_delta = d.delta;
                                        }
                                }
                                break;
                        }
                        case Event::EventType::set_ambient:
                        {
                                const Event::set_ambient& d = event.get<Event::set_ambient>();

                                glm::vec4 light = glm::convertSRGBToLinear(glm::vec4(1.0f) * d.ambient);
                                draw_program->set_light_a(light);
                                break;
                        }
                        case Event::EventType::set_diffuse:
                        {
                                const Event::set_diffuse& d = event.get<Event::set_diffuse>();

                                glm::vec4 light = glm::convertSRGBToLinear(glm::vec4(1.0f) * d.diffuse);
                                draw_program->set_light_d(light);
                                break;
                        }
                        case Event::EventType::set_specular:
                        {
                                const Event::set_specular& d = event.get<Event::set_specular>();

                                glm::vec4 light = glm::convertSRGBToLinear(glm::vec4(1.0f) * d.specular);
                                draw_program->set_light_s(light);
                                break;
                        }
                        case Event::EventType::set_clear_color:
                        {
                                const Event::set_clear_color& d = event.get<Event::set_clear_color>();

                                glm::vec3 color = glm::convertSRGBToLinear(d.clear_color);
                                glClearColor(color.r, color.g, color.b, 1);
                                text.set_color((luminosity(color) <= 0.5f) ? glm::vec3(1) : glm::vec3(0));
                                break;
                        }
                        case Event::EventType::set_default_color:
                        {
                                const Event::set_default_color& d = event.get<Event::set_default_color>();

                                glm::vec3 color = glm::convertSRGBToLinear(d.default_color);
                                draw_program->set_default_color(glm::vec4(color.r, color.g, color.b, 1));
                                break;
                        }
                        case Event::EventType::set_wireframe_color:
                        {
                                const Event::set_wireframe_color& d = event.get<Event::set_wireframe_color>();

                                glm::vec3 color = glm::convertSRGBToLinear(d.wireframe_color);
                                draw_program->set_wireframe_color(glm::vec4(color.r, color.g, color.b, 1));
                                break;
                        }
                        case Event::EventType::set_default_ns:
                        {
                                const Event::set_default_ns& d = event.get<Event::set_default_ns>();

                                draw_program->set_default_ns(d.default_ns);
                                break;
                        }
                        case Event::EventType::show_smooth:
                        {
                                const Event::show_smooth& d = event.get<Event::show_smooth>();

                                draw_program->set_show_smooth(d.show);
                                break;
                        }
                        case Event::EventType::show_wireframe:
                        {
                                const Event::show_wireframe& d = event.get<Event::show_wireframe>();

                                draw_program->set_show_wireframe(d.show);
                                break;
                        }
                        case Event::EventType::show_shadow:
                        {
                                const Event::show_shadow& d = event.get<Event::show_shadow>();

                                draw_program->set_show_shadow(d.show);
                                shadow_active = d.show;
                                break;
                        }
                        case Event::EventType::show_materials:
                        {
                                const Event::show_materials& d = event.get<Event::show_materials>();

                                draw_program->set_show_materials(d.show);
                                break;
                        }
                        case Event::EventType::show_effect:
                        {
                                const Event::show_effect& d = event.get<Event::show_effect>();

                                pencil_effect_active = d.show;
                                break;
                        }
                        case Event::EventType::show_dft:
                        {
                                const Event::show_dft& d = event.get<Event::show_dft>();

                                dft_active = d.show;
                                break;
                        }
                        case Event::EventType::set_dft_brightness:
                        {
                                const Event::set_dft_brightness& d = event.get<Event::set_dft_brightness>();

                                dft_brightness = d.dft_brightness;
                                if (dft_show)
                                {
                                        dft_show->set_brightness(d.dft_brightness);
                                }
                                break;
                        }
                        case Event::EventType::show_convex_hull_2d:
                        {
                                const Event::show_convex_hull_2d& d = event.get<Event::show_convex_hull_2d>();

                                convex_hull_2d_active = d.show;
                                if (convex_hull_2d)
                                {
                                        convex_hull_2d->reset_timer();
                                }
                                break;
                        }
                        case Event::EventType::show_optical_flow:
                        {
                                const Event::show_optical_flow& d = event.get<Event::show_optical_flow>();

                                optical_flow_active = d.show;
                                if (optical_flow)
                                {
                                        optical_flow->reset();
                                }
                                break;
                        }
                        }
                }

                while (true)
                {
                        sf::Event sf_event;

                        if (!wnd.pollEvent(sf_event))
                        {
                                break;
                        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
                        switch (sf_event.type)
                        {
                        case sf::Event::Closed:
                                break;
                        case sf::Event::KeyPressed:
                                switch (sf_event.key.code)
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
                                if (sf_event.mouseButton.x < width && sf_event.mouseButton.y < height &&
                                    (sf_event.mouseButton.button == sf::Mouse::Left ||
                                     sf_event.mouseButton.button == sf::Mouse::Right))
                                {
                                        mouse_pressed = true;
                                        mouse_pressed_shift = (sf_event.mouseButton.button == sf::Mouse::Left);
                                        mouse_x = sf_event.mouseButton.x;
                                        mouse_y = sf_event.mouseButton.y;
                                }
                                break;
                        case sf::Event::MouseButtonReleased:
                                if (sf_event.mouseButton.button == sf::Mouse::Left ||
                                    sf_event.mouseButton.button == sf::Mouse::Right)
                                {
                                        mouse_pressed = false;
                                }
                                break;
                        case sf::Event::MouseMoved:
                                new_mouse_x = sf_event.mouseMove.x;
                                new_mouse_y = sf_event.mouseMove.y;
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
                                                wheel_delta = sf_event.mouseWheelScroll.delta;
                                        }
                                }
                                break;
                        case sf::Event::Resized:
                                new_width = sf_event.size.width;
                                new_height = sf_event.size.height;
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
                                camera.rotate(delta_x, delta_y);
                        }
                        else
                        {
                                window_center.x -= delta_x * pixel_to_coord;
                                window_center.y -= -delta_y * pixel_to_coord;
                        }

                        matrix_change = true;
                }

                if (wheel_delta != 0)
                {
                        if ((wheel_delta < 0 && zoom_delta > ZOOM_EXP_MIN) || (wheel_delta > 0 && zoom_delta < ZOOM_EXP_MAX))
                        {
                                zoom_delta += wheel_delta;

                                glm::vec2 mouse_in_wnd((new_mouse_x - width * 0.5f) * pixel_to_coord,
                                                       (height * 0.5f - new_mouse_y) * pixel_to_coord);

                                window_center += mouse_in_wnd - mouse_in_wnd * std::pow(ZOOM_BASE, -wheel_delta);

                                pixel_to_coord = pixel_to_coord_no_zoom * std::pow(ZOOM_BASE, -zoom_delta);

                                matrix_change = true;
                        }
                        wheel_delta = 0;
                }

                if (window_width != new_width || window_height != new_height || dft_active_old != dft_active)
                {
                        // Чтобы в случае исключений не остались объекты от прошлых размеров окна,
                        // вначале нужно удалить все объекты.

                        draw_program->free_buffers();

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
                        const glm::mat4 plane_matrix =
                                glm::scale(glm::mat4(1), glm::vec3(2.0f / window_width, -2.0f / window_height, 1)) *
                                glm::translate(glm::mat4(1), glm::vec3(-window_width / 2.0f, -window_height / 2.0f, 0));

                        draw_program->set_size(width, height);

                        int dft_pos_x = (window_width & 1) ? (width + 1) : width;
                        int dft_pos_y = 0;
                        dft_show = std::make_unique<DFTShow>(width, height, dft_pos_x, dft_pos_y, plane_matrix, buffer_sRGB);
                        dft_show->set_brightness(dft_brightness);

                        pencil_effect = std::make_unique<PencilEffect>(draw_program->get_color_buffer_texture(),
                                                                       draw_program->get_object_texture());

                        optical_flow = std::make_unique<OpticalFlow>(width, height, plane_matrix);

                        convex_hull_2d = std::make_unique<ConvexHull2D>(draw_program->get_object_texture(), plane_matrix);
                }

                if (default_view)
                {
                        default_view = false;

                        zoom_delta = 0;
                        window_center = glm::vec2(0);
                        pixel_to_coord = pixel_to_coord_no_zoom = 2.0f / std::min(width, height);
                        camera.set(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));

                        matrix_change = true;
                }

                if (matrix_change)
                {
                        glm::mat4 shadow_matrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f) *
                                                  glm::lookAt(glm::vec3(0, 0, 0), -camera.light_dir(), camera.light_up());

                        glm::mat4 main_matrix =
                                glm::ortho(-0.5f * width * pixel_to_coord, 0.5f * width * pixel_to_coord,
                                           -0.5f * height * pixel_to_coord, 0.5f * height * pixel_to_coord) *
                                glm::translate(glm::mat4(1), glm::vec3(-window_center.x, -window_center.y, 0.0f)) *
                                glm::lookAt(glm::vec3(0, 0, 0), -camera.dir(), camera.up());

                        draw_program->set_matrices(shadow_matrix, main_matrix);

                        draw_program->set_light_direction(camera.light_dir());
                        draw_program->set_camera_direction(camera.dir());
                }

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                //

                glEnable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);

                // Если pencil_effect_active, то рисование в цветной буфер
                draw_program->draw(objects.get_object(), objects.get_scale_object(), shadow_active, pencil_effect_active);

                //

                // Рисование из цветного буфера в буфер экрана
                if (objects.get_object() && pencil_effect_active && pencil_effect)
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

std::unique_ptr<IShow> create_show(IShowCallback* cb, WindowID win_parent, glm::vec3 clear_color, glm::vec3 default_color,
                                   glm::vec3 wireframe_color, bool with_smooth, bool with_wireframe, bool with_shadow,
                                   bool with_materials, bool with_effect, bool with_dft, bool with_convex_hull,
                                   bool with_optical_flow, float ambient, float diffuse, float specular, float dft_brightness,
                                   float default_ns)
{
        return std::make_unique<ShowObject>(cb, win_parent, clear_color, default_color, wireframe_color, with_smooth,
                                            with_wireframe, with_shadow, with_materials, with_effect, with_dft, with_convex_hull,
                                            with_optical_flow, ambient, diffuse, specular, dft_brightness, default_ns);
}
