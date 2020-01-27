/*
Copyright (C) 2017-2020 Topological Manifold

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

#pragma once

#include "com/thread.h"
#include "com/variant.h"
#include "show/interface.h"

// Класс для вызова функций не напрямую, а через очередь.
// Это нужно для работы функций в другом потоке.

class EventQueue final : public Show
{
        struct Event final
        {
                struct add_object final
                {
                        std::shared_ptr<const Obj<3>> obj;
                        int id;
                        int scale_id;
                        add_object(const std::shared_ptr<const Obj<3>>& obj_, int id_, int scale_id_)
                                : obj(obj_), id(id_), scale_id(scale_id_)
                        {
                        }
                };
                struct delete_object final
                {
                        int id;
                        explicit delete_object(int id_) : id(id_)
                        {
                        }
                };
                struct show_object final
                {
                        int id;
                        explicit show_object(int id_) : id(id_)
                        {
                        }
                };
                struct delete_all_objects final
                {
                };
                struct reset_view final
                {
                };
                struct set_ambient final
                {
                        double ambient;
                        explicit set_ambient(double v) : ambient(v)
                        {
                        }
                };
                struct set_diffuse final
                {
                        double diffuse;
                        explicit set_diffuse(double v) : diffuse(v)
                        {
                        }
                };
                struct set_specular final
                {
                        double specular;
                        explicit set_specular(double v) : specular(v)
                        {
                        }
                };
                struct set_background_color final
                {
                        Color background_color;
                        explicit set_background_color(const Color& v) : background_color(v)
                        {
                        }
                };
                struct set_default_color final
                {
                        Color default_color;
                        explicit set_default_color(const Color& v) : default_color(v)
                        {
                        }
                };
                struct set_wireframe_color final
                {
                        Color wireframe_color;
                        explicit set_wireframe_color(const Color& v) : wireframe_color(v)
                        {
                        }
                };
                struct set_default_ns final
                {
                        double default_ns;
                        explicit set_default_ns(double v) : default_ns(v)
                        {
                        }
                };
                struct show_smooth final
                {
                        bool show;
                        explicit show_smooth(bool v) : show(v)
                        {
                        }
                };
                struct show_wireframe final
                {
                        bool show;
                        explicit show_wireframe(bool v) : show(v)
                        {
                        }
                };
                struct show_shadow final
                {
                        bool show;
                        explicit show_shadow(bool v) : show(v)
                        {
                        }
                };
                struct show_fog final
                {
                        bool show;
                        explicit show_fog(bool v) : show(v)
                        {
                        }
                };
                struct show_materials final
                {
                        bool show;
                        explicit show_materials(bool v) : show(v)
                        {
                        }
                };
                struct show_fps final
                {
                        bool show;
                        explicit show_fps(bool v) : show(v)
                        {
                        }
                };
                struct show_pencil_sketch final
                {
                        bool show;
                        explicit show_pencil_sketch(bool v) : show(v)
                        {
                        }
                };
                struct show_dft final
                {
                        bool show;
                        explicit show_dft(bool v) : show(v)
                        {
                        }
                };
                struct set_dft_brightness final
                {
                        double dft_brightness;
                        explicit set_dft_brightness(double v) : dft_brightness(v)
                        {
                        }
                };
                struct set_dft_background_color final
                {
                        Color color;
                        explicit set_dft_background_color(const Color& c) : color(c)
                        {
                        }
                };
                struct set_dft_color final
                {
                        Color color;
                        explicit set_dft_color(const Color& c) : color(c)
                        {
                        }
                };
                struct show_convex_hull_2d final
                {
                        bool show;
                        explicit show_convex_hull_2d(bool v) : show(v)
                        {
                        }
                };
                struct show_optical_flow final
                {
                        bool show;
                        explicit show_optical_flow(bool v) : show(v)
                        {
                        }
                };
                struct set_vertical_sync final
                {
                        bool enable;
                        explicit set_vertical_sync(bool v) : enable(v)
                        {
                        }
                };
                struct set_shadow_zoom final
                {
                        double zoom;
                        explicit set_shadow_zoom(double v) : zoom(v)
                        {
                        }
                };
                struct clip_plane_show final
                {
                        double position;
                        explicit clip_plane_show(double p) : position(p)
                        {
                        }
                };
                struct clip_plane_position final
                {
                        double position;
                        explicit clip_plane_position(double p) : position(p)
                        {
                        }
                };
                struct clip_plane_hide final
                {
                };
                struct mouse_press final
                {
                        int x, y;
                        ShowMouseButton button;
                        mouse_press(int x_coord, int y_coord, ShowMouseButton b) : x(x_coord), y(y_coord), button(b)
                        {
                        }
                };
                struct mouse_release final
                {
                        int x, y;
                        ShowMouseButton button;
                        mouse_release(int x_coord, int y_coord, ShowMouseButton b) : x(x_coord), y(y_coord), button(b)
                        {
                        }
                };
                struct mouse_move final
                {
                        int x, y;
                        mouse_move(int x_coord, int y_coord) : x(x_coord), y(y_coord)
                        {
                        }
                };
                struct mouse_wheel final
                {
                        int x, y;
                        double delta;
                        mouse_wheel(int x_coord, int y_coord, double d) : x(x_coord), y(y_coord), delta(d)
                        {
                        }
                };
                struct window_resize final
                {
                        int x, y;
                        window_resize(int x_coord, int y_coord) : x(x_coord), y(y_coord)
                        {
                        }
                };

                using EventType = Variant<add_object, delete_all_objects, delete_object, reset_view, set_ambient,
                                          set_background_color, set_default_color, set_default_ns, set_dft_background_color,
                                          set_dft_brightness, set_dft_color, set_diffuse, set_shadow_zoom, set_specular,
                                          set_vertical_sync, set_wireframe_color, show_convex_hull_2d, show_dft, show_fps,
                                          show_pencil_sketch, show_fog, show_materials, show_object, show_optical_flow,
                                          show_shadow, show_smooth, show_wireframe, clip_plane_show, clip_plane_position,
                                          clip_plane_hide, mouse_press, mouse_release, mouse_move, mouse_wheel, window_resize>;

                template <typename... Args>
                explicit Event(Args&&... args) : m_event(std::forward<Args>(args)...)
                {
                }

                const EventType& event() const
                {
                        return m_event;
                }

        private:
                EventType m_event;
        };

        class Visitor final
        {
                Show* m_show;

        public:
                explicit Visitor(Show* show) : m_show(show)
                {
                }

                void operator()(const Event::add_object& d)
                {
                        m_show->add_object(d.obj, d.id, d.scale_id);
                }
                void operator()(const Event::delete_object& d)
                {
                        m_show->delete_object(d.id);
                }
                void operator()(const Event::show_object& d)
                {
                        m_show->show_object(d.id);
                }
                void operator()(const Event::delete_all_objects&)
                {
                        m_show->delete_all_objects();
                }
                void operator()(const Event::reset_view)
                {
                        m_show->reset_view();
                }
                void operator()(const Event::set_ambient& d)
                {
                        m_show->set_ambient(d.ambient);
                }
                void operator()(const Event::set_diffuse& d)
                {
                        m_show->set_diffuse(d.diffuse);
                }
                void operator()(const Event::set_specular& d)
                {
                        m_show->set_specular(d.specular);
                }
                void operator()(const Event::set_background_color& d)
                {
                        m_show->set_background_color(d.background_color);
                }
                void operator()(const Event::set_default_color& d)
                {
                        m_show->set_default_color(d.default_color);
                }
                void operator()(const Event::set_wireframe_color& d)
                {
                        m_show->set_wireframe_color(d.wireframe_color);
                }
                void operator()(const Event::set_default_ns& d)
                {
                        m_show->set_default_ns(d.default_ns);
                }
                void operator()(const Event::show_smooth& d)
                {
                        m_show->show_smooth(d.show);
                }
                void operator()(const Event::show_wireframe& d)
                {
                        m_show->show_wireframe(d.show);
                }
                void operator()(const Event::show_shadow& d)
                {
                        m_show->show_shadow(d.show);
                }
                void operator()(const Event::show_fog& d)
                {
                        m_show->show_fog(d.show);
                }
                void operator()(const Event::show_materials& d)
                {
                        m_show->show_materials(d.show);
                }
                void operator()(const Event::show_fps& d)
                {
                        m_show->show_fps(d.show);
                }
                void operator()(const Event::show_pencil_sketch& d)
                {
                        m_show->show_pencil_sketch(d.show);
                }
                void operator()(const Event::show_dft& d)
                {
                        m_show->show_dft(d.show);
                }
                void operator()(const Event::set_dft_brightness& d)
                {
                        m_show->set_dft_brightness(d.dft_brightness);
                }
                void operator()(const Event::set_dft_background_color& d)
                {
                        m_show->set_dft_background_color(d.color);
                }
                void operator()(const Event::set_dft_color& d)
                {
                        m_show->set_dft_color(d.color);
                }
                void operator()(const Event::show_convex_hull_2d& d)
                {
                        m_show->show_convex_hull_2d(d.show);
                }
                void operator()(const Event::show_optical_flow& d)
                {
                        m_show->show_optical_flow(d.show);
                }
                void operator()(const Event::set_vertical_sync& d)
                {
                        m_show->set_vertical_sync(d.enable);
                }
                void operator()(const Event::set_shadow_zoom& d)
                {
                        m_show->set_shadow_zoom(d.zoom);
                }
                void operator()(const Event::clip_plane_show& d)
                {
                        m_show->clip_plane_show(d.position);
                }
                void operator()(const Event::clip_plane_position& d)
                {
                        m_show->clip_plane_position(d.position);
                }
                void operator()(const Event::clip_plane_hide&)
                {
                        m_show->clip_plane_hide();
                }
                void operator()(const Event::mouse_press& d)
                {
                        m_show->mouse_press(d.x, d.y, d.button);
                }
                void operator()(const Event::mouse_release& d)
                {
                        m_show->mouse_release(d.x, d.y, d.button);
                }
                void operator()(const Event::mouse_move& d)
                {
                        m_show->mouse_move(d.x, d.y);
                }
                void operator()(const Event::mouse_wheel& d)
                {
                        m_show->mouse_wheel(d.x, d.y, d.delta);
                }
                void operator()(const Event::window_resize& d)
                {
                        m_show->window_resize(d.x, d.y);
                }
        };

        ThreadQueue<Event> m_event_queue;
        Show* m_show = nullptr;
        mutable SpinLock m_lock;

        void add_object(const std::shared_ptr<const Obj<3>>& obj_ptr, int id, int scale_id) override
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
        void delete_all_objects() override
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
        void set_background_color(const Color& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_background_color>, c);
        }
        void set_default_color(const Color& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_color>, c);
        }
        void set_wireframe_color(const Color& c) override
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
        void show_fog(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_fog>, v);
        }
        void show_materials(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_materials>, v);
        }
        void show_fps(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_fps>, v);
        }
        void show_pencil_sketch(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_pencil_sketch>, v);
        }
        void show_dft(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_dft>, v);
        }
        void set_dft_brightness(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_brightness>, v);
        }
        void set_dft_background_color(const Color& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_background_color>, c);
        }
        void set_dft_color(const Color& c) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_color>, c);
        }
        void show_convex_hull_2d(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_convex_hull_2d>, v);
        }
        void show_optical_flow(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::show_optical_flow>, v);
        }
        void set_vertical_sync(bool v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_vertical_sync>, v);
        }
        void set_shadow_zoom(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::set_shadow_zoom>, v);
        }
        void clip_plane_show(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::clip_plane_show>, v);
        }
        void clip_plane_position(double v) override
        {
                m_event_queue.emplace(std::in_place_type<Event::clip_plane_position>, v);
        }
        void clip_plane_hide() override
        {
                m_event_queue.emplace(std::in_place_type<Event::clip_plane_hide>);
        }
        void mouse_press(int x, int y, ShowMouseButton button) override
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_press>, x, y, button);
        }
        void mouse_release(int x, int y, ShowMouseButton button) override
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_release>, x, y, button);
        }
        void mouse_move(int x, int y) override
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_move>, x, y);
        }
        void mouse_wheel(int x, int y, double delta) override
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_wheel>, x, y, delta);
        }
        void window_resize(int x, int y) override
        {
                m_event_queue.emplace(std::in_place_type<Event::window_resize>, x, y);
        }

        ShowCameraInfo camera_information() const override
        {
                std::lock_guard lg(m_lock);
                if (!m_show)
                {
                        error("No show");
                }
                return m_show->camera_information();
        }
        double object_size() const override
        {
                std::lock_guard lg(m_lock);
                if (!m_show)
                {
                        error("No show");
                }
                return m_show->object_size();
        }
        vec3 object_position() const override
        {
                std::lock_guard lg(m_lock);
                if (!m_show)
                {
                        error("No show");
                }
                return m_show->object_position();
        }

public:
        void set_show(Show* show)
        {
                std::lock_guard lg(m_lock);
                m_show = show;
        }

        void pull_and_dispatch_events(Show* show)
        {
                std::optional<Event> event;
                while ((event = m_event_queue.pop()))
                {
                        visit(Visitor(show), event->event());
                }
        }
};
