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

#pragma once

#include "com/thread.h"
#include "com/variant.h"
#include "show/show.h"

// Класс для вызова функций не напрямую, а через очередь.
// Это нужно для работы функций в другом потоке.

class EventQueue : public IShow
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
                        delete_object(int id_) : id(id_)
                        {
                        }
                };
                struct show_object final
                {
                        int id;
                        show_object(int id_) : id(id_)
                        {
                        }
                };
                struct delete_all_objects final
                {
                        delete_all_objects()
                        {
                        }
                };
                struct parent_resized final
                {
                        parent_resized()
                        {
                        }
                };
                struct mouse_wheel final
                {
                        double delta;
                        mouse_wheel(double d) : delta(d)
                        {
                        }
                };
                struct toggle_fullscreen final
                {
                        toggle_fullscreen()
                        {
                        }
                };
                struct reset_view final
                {
                        reset_view()
                        {
                        }
                };
                struct set_ambient final
                {
                        double ambient;
                        set_ambient(double v) : ambient(v)
                        {
                        }
                };
                struct set_diffuse final
                {
                        double diffuse;
                        set_diffuse(double v) : diffuse(v)
                        {
                        }
                };
                struct set_specular final
                {
                        double specular;
                        set_specular(double v) : specular(v)
                        {
                        }
                };
                struct set_background_color_rgb final
                {
                        Color background_color;
                        set_background_color_rgb(const Color& v) : background_color(v)
                        {
                        }
                };
                struct set_default_color_rgb final
                {
                        Color default_color;
                        set_default_color_rgb(const Color& v) : default_color(v)
                        {
                        }
                };
                struct set_wireframe_color_rgb final
                {
                        Color wireframe_color;
                        set_wireframe_color_rgb(const Color& v) : wireframe_color(v)
                        {
                        }
                };
                struct set_default_ns final
                {
                        double default_ns;
                        set_default_ns(double v) : default_ns(v)
                        {
                        }
                };
                struct show_smooth final
                {
                        bool show;
                        show_smooth(bool v) : show(v)
                        {
                        }
                };
                struct show_wireframe final
                {
                        bool show;
                        show_wireframe(bool v) : show(v)
                        {
                        }
                };
                struct show_shadow final
                {
                        bool show;
                        show_shadow(bool v) : show(v)
                        {
                        }
                };
                struct show_fog final
                {
                        bool show;
                        show_fog(bool v) : show(v)
                        {
                        }
                };
                struct show_materials final
                {
                        bool show;
                        show_materials(bool v) : show(v)
                        {
                        }
                };
                struct show_fps final
                {
                        bool show;
                        show_fps(bool v) : show(v)
                        {
                        }
                };
                struct show_effect final
                {
                        bool show;
                        show_effect(bool v) : show(v)
                        {
                        }
                };
                struct show_dft final
                {
                        bool show;
                        show_dft(bool v) : show(v)
                        {
                        }
                };
                struct set_dft_brightness final
                {
                        double dft_brightness;
                        set_dft_brightness(double v) : dft_brightness(v)
                        {
                        }
                };
                struct set_dft_background_color final
                {
                        Color color;
                        set_dft_background_color(const Color& c) : color(c)
                        {
                        }
                };
                struct set_dft_color final
                {
                        Color color;
                        set_dft_color(const Color& c) : color(c)
                        {
                        }
                };
                struct show_convex_hull_2d final
                {
                        bool show;
                        show_convex_hull_2d(bool v) : show(v)
                        {
                        }
                };
                struct show_optical_flow final
                {
                        bool show;
                        show_optical_flow(bool v) : show(v)
                        {
                        }
                };
                struct set_vertical_sync final
                {
                        bool enable;
                        set_vertical_sync(bool v) : enable(v)
                        {
                        }
                };
                struct set_shadow_zoom final
                {
                        double zoom;
                        set_shadow_zoom(double v) : zoom(v)
                        {
                        }
                };

                const Variant<add_object, delete_all_objects, delete_object, mouse_wheel, parent_resized, reset_view, set_ambient,
                              set_background_color_rgb, set_default_color_rgb, set_default_ns, set_dft_background_color,
                              set_dft_brightness, set_dft_color, set_diffuse, set_shadow_zoom, set_specular, set_vertical_sync,
                              set_wireframe_color_rgb, show_convex_hull_2d, show_dft, show_fps, show_effect, show_fog,
                              show_materials, show_object, show_optical_flow, show_shadow, show_smooth, show_wireframe,
                              toggle_fullscreen>
                        event;

                template <typename... Args>
                Event(Args&&... args) : event(std::forward<Args>(args)...)
                {
                }
        };

        ThreadQueue<Event> m_event_queue;

protected:
        void add_object(const std::shared_ptr<const Obj<3>>& obj_ptr, int id, int scale_id) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::add_object>, obj_ptr, id, scale_id);
        }
        void delete_object(int id) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::delete_object>, id);
        }
        void show_object(int id) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_object>, id);
        }
        void delete_all_objects() override final
        {
                m_event_queue.emplace(std::in_place_type<Event::delete_all_objects>);
        }
        void reset_view() override final
        {
                m_event_queue.emplace(std::in_place_type<Event::reset_view>);
        }
        void set_ambient(double v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_ambient>, v);
        }
        void set_diffuse(double v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_diffuse>, v);
        }
        void set_specular(double v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_specular>, v);
        }
        void set_background_color_rgb(const Color& c) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_background_color_rgb>, c);
        }
        void set_default_color_rgb(const Color& c) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_color_rgb>, c);
        }
        void set_wireframe_color_rgb(const Color& c) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_wireframe_color_rgb>, c);
        }
        void set_default_ns(double ns) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_default_ns>, ns);
        }
        void show_smooth(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_smooth>, v);
        }
        void show_wireframe(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_wireframe>, v);
        }
        void show_shadow(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_shadow>, v);
        }
        void show_fog(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_fog>, v);
        }
        void show_materials(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_materials>, v);
        }
        void show_fps(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_fps>, v);
        }
        void show_effect(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_effect>, v);
        }
        void show_dft(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_dft>, v);
        }
        void set_dft_brightness(double v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_brightness>, v);
        }
        void set_dft_background_color(const Color& c) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_background_color>, c);
        }
        void set_dft_color(const Color& c) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_dft_color>, c);
        }
        void show_convex_hull_2d(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_convex_hull_2d>, v);
        }
        void show_optical_flow(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::show_optical_flow>, v);
        }
        void parent_resized() override final
        {
                m_event_queue.emplace(std::in_place_type<Event::parent_resized>);
        }
        void mouse_wheel(double delta) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::mouse_wheel>, delta);
        }
        void toggle_fullscreen() override final
        {
                m_event_queue.emplace(std::in_place_type<Event::toggle_fullscreen>);
        }
        void set_vertical_sync(bool v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_vertical_sync>, v);
        }
        void set_shadow_zoom(double v) override final
        {
                m_event_queue.emplace(std::in_place_type<Event::set_shadow_zoom>, v);
        }

        virtual void direct_add_object(const std::shared_ptr<const Obj<3>>&, int id, int scale_id) = 0;
        virtual void direct_delete_object(int id) = 0;
        virtual void direct_delete_all_objects() = 0;
        virtual void direct_show_object(int id) = 0;
        virtual void direct_parent_resized() = 0;
        virtual void direct_mouse_wheel(double) = 0;
        virtual void direct_toggle_fullscreen() = 0;
        virtual void direct_reset_view() = 0;
        virtual void direct_set_ambient(double) = 0;
        virtual void direct_set_diffuse(double) = 0;
        virtual void direct_set_specular(double) = 0;
        virtual void direct_set_background_color_rgb(const Color&) = 0;
        virtual void direct_set_default_color_rgb(const Color&) = 0;
        virtual void direct_set_wireframe_color_rgb(const Color&) = 0;
        virtual void direct_set_default_ns(double) = 0;
        virtual void direct_show_smooth(bool) = 0;
        virtual void direct_show_wireframe(bool) = 0;
        virtual void direct_show_shadow(bool) = 0;
        virtual void direct_show_fog(bool) = 0;
        virtual void direct_show_materials(bool) = 0;
        virtual void direct_show_fps(bool) = 0;
        virtual void direct_show_effect(bool) = 0;
        virtual void direct_show_dft(bool) = 0;
        virtual void direct_set_dft_brightness(double) = 0;
        virtual void direct_set_dft_background_color(const Color&) = 0;
        virtual void direct_set_dft_color(const Color&) = 0;
        virtual void direct_show_convex_hull_2d(bool) = 0;
        virtual void direct_show_optical_flow(bool) = 0;
        virtual void direct_set_vertical_sync(bool v) = 0;
        virtual void direct_set_shadow_zoom(double v) = 0;

private:
        class Visitor
        {
                EventQueue& m_f;

        public:
                Visitor(EventQueue& event_queue) : m_f(event_queue)
                {
                }

                void operator()(const Event::add_object& d)
                {
                        m_f.direct_add_object(d.obj, d.id, d.scale_id);
                }
                void operator()(const Event::delete_object& d)
                {
                        m_f.direct_delete_object(d.id);
                }
                void operator()(const Event::show_object& d)
                {
                        m_f.direct_show_object(d.id);
                }
                void operator()(const Event::delete_all_objects&)
                {
                        m_f.direct_delete_all_objects();
                }
                void operator()(const Event::parent_resized&)
                {
                        m_f.direct_parent_resized();
                }
                void operator()(const Event::toggle_fullscreen&)
                {
                        m_f.direct_toggle_fullscreen();
                }
                void operator()(const Event::reset_view)
                {
                        m_f.direct_reset_view();
                }
                void operator()(const Event::mouse_wheel& d)
                {
                        m_f.direct_mouse_wheel(d.delta);
                }
                void operator()(const Event::set_ambient& d)
                {
                        m_f.direct_set_ambient(d.ambient);
                }
                void operator()(const Event::set_diffuse& d)
                {
                        m_f.direct_set_diffuse(d.diffuse);
                }
                void operator()(const Event::set_specular& d)
                {
                        m_f.direct_set_specular(d.specular);
                }
                void operator()(const Event::set_background_color_rgb& d)
                {
                        m_f.direct_set_background_color_rgb(d.background_color);
                }
                void operator()(const Event::set_default_color_rgb& d)
                {
                        m_f.direct_set_default_color_rgb(d.default_color);
                }
                void operator()(const Event::set_wireframe_color_rgb& d)
                {
                        m_f.direct_set_wireframe_color_rgb(d.wireframe_color);
                }
                void operator()(const Event::set_default_ns& d)
                {
                        m_f.direct_set_default_ns(d.default_ns);
                }
                void operator()(const Event::show_smooth& d)
                {
                        m_f.direct_show_smooth(d.show);
                }
                void operator()(const Event::show_wireframe& d)
                {
                        m_f.direct_show_wireframe(d.show);
                }
                void operator()(const Event::show_shadow& d)
                {
                        m_f.direct_show_shadow(d.show);
                }
                void operator()(const Event::show_fog& d)
                {
                        m_f.direct_show_fog(d.show);
                }
                void operator()(const Event::show_materials& d)
                {
                        m_f.direct_show_materials(d.show);
                }
                void operator()(const Event::show_fps& d)
                {
                        m_f.direct_show_fps(d.show);
                }
                void operator()(const Event::show_effect& d)
                {
                        m_f.direct_show_effect(d.show);
                }
                void operator()(const Event::show_dft& d)
                {
                        m_f.direct_show_dft(d.show);
                }
                void operator()(const Event::set_dft_brightness& d)
                {
                        m_f.direct_set_dft_brightness(d.dft_brightness);
                }
                void operator()(const Event::set_dft_background_color& d)
                {
                        m_f.direct_set_dft_background_color(d.color);
                }
                void operator()(const Event::set_dft_color& d)
                {
                        m_f.direct_set_dft_color(d.color);
                }
                void operator()(const Event::show_convex_hull_2d& d)
                {
                        m_f.direct_show_convex_hull_2d(d.show);
                }
                void operator()(const Event::show_optical_flow& d)
                {
                        m_f.direct_show_optical_flow(d.show);
                }
                void operator()(const Event::set_vertical_sync& d)
                {
                        m_f.direct_set_vertical_sync(d.enable);
                }
                void operator()(const Event::set_shadow_zoom& d)
                {
                        m_f.direct_set_shadow_zoom(d.zoom);
                }
        };

protected:
        void pull_and_dispatch_events()
        {
                while (true)
                {
                        std::optional<Event> event(m_event_queue.pop());

                        if (!event)
                        {
                                return;
                        }

                        visit(Visitor(*this), event->event);
                }
        }
};
