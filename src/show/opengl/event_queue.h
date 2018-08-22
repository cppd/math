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

#include "event.h"
#include "show.h"

#include "com/error.h"
#include "com/thread.h"

// Класс для вызова функций не напрямую, а через очередь.
// Это нужно для работы функций в другом потоке.

class EventQueue : public IShow
{
        using Event = show_event_queue_implementation::Event;

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
                EventQueue& m_event_queue;

        public:
                Visitor(EventQueue& event_queue) : m_event_queue(event_queue)
                {
                }

                void operator()(const Event::add_object& d)
                {
                        m_event_queue.direct_add_object(d.obj, d.id, d.scale_id);
                }
                void operator()(const Event::delete_object& d)
                {
                        m_event_queue.direct_delete_object(d.id);
                }
                void operator()(const Event::show_object& d)
                {
                        m_event_queue.direct_show_object(d.id);
                }
                void operator()(const Event::delete_all_objects&)
                {
                        m_event_queue.direct_delete_all_objects();
                }
                void operator()(const Event::parent_resized&)
                {
                        m_event_queue.direct_parent_resized();
                }
                void operator()(const Event::toggle_fullscreen&)
                {
                        m_event_queue.direct_toggle_fullscreen();
                }
                void operator()(const Event::reset_view)
                {
                        m_event_queue.direct_reset_view();
                }
                void operator()(const Event::mouse_wheel& d)
                {
                        m_event_queue.direct_mouse_wheel(d.delta);
                }
                void operator()(const Event::set_ambient& d)
                {
                        m_event_queue.direct_set_ambient(d.ambient);
                }
                void operator()(const Event::set_diffuse& d)
                {
                        m_event_queue.direct_set_diffuse(d.diffuse);
                }
                void operator()(const Event::set_specular& d)
                {
                        m_event_queue.direct_set_specular(d.specular);
                }
                void operator()(const Event::set_background_color_rgb& d)
                {
                        m_event_queue.direct_set_background_color_rgb(d.background_color);
                }
                void operator()(const Event::set_default_color_rgb& d)
                {
                        m_event_queue.direct_set_default_color_rgb(d.default_color);
                }
                void operator()(const Event::set_wireframe_color_rgb& d)
                {
                        m_event_queue.direct_set_wireframe_color_rgb(d.wireframe_color);
                }
                void operator()(const Event::set_default_ns& d)
                {
                        m_event_queue.direct_set_default_ns(d.default_ns);
                }
                void operator()(const Event::show_smooth& d)
                {
                        m_event_queue.direct_show_smooth(d.show);
                }
                void operator()(const Event::show_wireframe& d)
                {
                        m_event_queue.direct_show_wireframe(d.show);
                }
                void operator()(const Event::show_shadow& d)
                {
                        m_event_queue.direct_show_shadow(d.show);
                }
                void operator()(const Event::show_fog& d)
                {
                        m_event_queue.direct_show_fog(d.show);
                }
                void operator()(const Event::show_materials& d)
                {
                        m_event_queue.direct_show_materials(d.show);
                }
                void operator()(const Event::show_effect& d)
                {
                        m_event_queue.direct_show_effect(d.show);
                }
                void operator()(const Event::show_dft& d)
                {
                        m_event_queue.direct_show_dft(d.show);
                }
                void operator()(const Event::set_dft_brightness& d)
                {
                        m_event_queue.direct_set_dft_brightness(d.dft_brightness);
                }
                void operator()(const Event::set_dft_background_color& d)
                {
                        m_event_queue.direct_set_dft_background_color(d.color);
                }
                void operator()(const Event::set_dft_color& d)
                {
                        m_event_queue.direct_set_dft_color(d.color);
                }
                void operator()(const Event::show_convex_hull_2d& d)
                {
                        m_event_queue.direct_show_convex_hull_2d(d.show);
                }
                void operator()(const Event::show_optical_flow& d)
                {
                        m_event_queue.direct_show_optical_flow(d.show);
                }
                void operator()(const Event::set_vertical_sync& d)
                {
                        m_event_queue.direct_set_vertical_sync(d.enable);
                }
                void operator()(const Event::set_shadow_zoom& d)
                {
                        m_event_queue.direct_set_shadow_zoom(d.zoom);
                }
        };

protected:
        bool pull_and_dispatch_event()
        {
                const std::optional<Event> event(m_event_queue.pop());

                if (!event)
                {
                        return false;
                }
                else
                {
                        visit(Visitor(*this), event->event);

                        return true;
                }
        }
};
