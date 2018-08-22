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

        bool pull_and_dispatch_event()
        {
                std::optional<Event> event(m_event_queue.pop());

                if (!event)
                {
                        return false;
                }

                switch (event->type())
                {
                case Event::Type::AddObject:
                {
                        const Event::add_object& d = event->get<Event::add_object>();
                        direct_add_object(d.obj, d.id, d.scale_id);
                        return true;
                }
                case Event::Type::DeleteObject:
                {
                        const Event::delete_object& d = event->get<Event::delete_object>();
                        direct_delete_object(d.id);
                        return true;
                }
                case Event::Type::ShowObject:
                {
                        const Event::show_object& d = event->get<Event::show_object>();
                        direct_show_object(d.id);
                        return true;
                }
                case Event::Type::DeleteAllObjects:
                {
                        direct_delete_all_objects();
                        return true;
                }
                case Event::Type::ParentResized:
                {
                        direct_parent_resized();
                        return true;
                }
                case Event::Type::ToggleFullscreen:
                {
                        direct_toggle_fullscreen();
                        return true;
                }
                case Event::Type::ResetView:
                {
                        direct_reset_view();
                        return true;
                }
                case Event::Type::MouseWheel:
                {
                        const Event::mouse_wheel& d = event->get<Event::mouse_wheel>();
                        direct_mouse_wheel(d.delta);
                        return true;
                }
                case Event::Type::SetAmbient:
                {
                        const Event::set_ambient& d = event->get<Event::set_ambient>();
                        direct_set_ambient(d.ambient);
                        return true;
                }
                case Event::Type::SetDiffuse:
                {
                        const Event::set_diffuse& d = event->get<Event::set_diffuse>();
                        direct_set_diffuse(d.diffuse);
                        return true;
                }
                case Event::Type::SetSpecular:
                {
                        const Event::set_specular& d = event->get<Event::set_specular>();
                        direct_set_specular(d.specular);
                        return true;
                }
                case Event::Type::SetBackgroundColorRGB:
                {
                        const Event::set_background_color_rgb& d = event->get<Event::set_background_color_rgb>();
                        direct_set_background_color_rgb(d.background_color);
                        return true;
                }
                case Event::Type::SetDefaultColorRGB:
                {
                        const Event::set_default_color_rgb& d = event->get<Event::set_default_color_rgb>();
                        direct_set_default_color_rgb(d.default_color);
                        return true;
                }
                case Event::Type::SetWireframeColorRGB:
                {
                        const Event::set_wireframe_color_rgb& d = event->get<Event::set_wireframe_color_rgb>();
                        direct_set_wireframe_color_rgb(d.wireframe_color);
                        return true;
                }
                case Event::Type::SetDefaultNs:
                {
                        const Event::set_default_ns& d = event->get<Event::set_default_ns>();
                        direct_set_default_ns(d.default_ns);
                        return true;
                }
                case Event::Type::ShowSmooth:
                {
                        const Event::show_smooth& d = event->get<Event::show_smooth>();
                        direct_show_smooth(d.show);
                        return true;
                }
                case Event::Type::ShowWireframe:
                {
                        const Event::show_wireframe& d = event->get<Event::show_wireframe>();
                        direct_show_wireframe(d.show);
                        return true;
                }
                case Event::Type::ShowShadow:
                {
                        const Event::show_shadow& d = event->get<Event::show_shadow>();
                        direct_show_shadow(d.show);
                        return true;
                }
                case Event::Type::ShowFog:
                {
                        const Event::show_fog& d = event->get<Event::show_fog>();
                        direct_show_fog(d.show);
                        return true;
                }
                case Event::Type::ShowMaterials:
                {
                        const Event::show_materials& d = event->get<Event::show_materials>();
                        direct_show_materials(d.show);
                        return true;
                }
                case Event::Type::ShowEffect:
                {
                        const Event::show_effect& d = event->get<Event::show_effect>();
                        direct_show_effect(d.show);
                        return true;
                }
                case Event::Type::ShowDft:
                {
                        const Event::show_dft& d = event->get<Event::show_dft>();
                        direct_show_dft(d.show);
                        return true;
                }
                case Event::Type::SetDftBrightness:
                {
                        const Event::set_dft_brightness& d = event->get<Event::set_dft_brightness>();
                        direct_set_dft_brightness(d.dft_brightness);
                        return true;
                }
                case Event::Type::SetDftBackgroundColor:
                {
                        const Event::set_dft_background_color& d = event->get<Event::set_dft_background_color>();
                        direct_set_dft_background_color(d.color);
                        return true;
                }
                case Event::Type::SetDftColor:
                {
                        const Event::set_dft_color& d = event->get<Event::set_dft_color>();
                        direct_set_dft_color(d.color);
                        return true;
                }
                case Event::Type::ShowConvexHull2d:
                {
                        const Event::show_convex_hull_2d& d = event->get<Event::show_convex_hull_2d>();
                        direct_show_convex_hull_2d(d.show);
                        return true;
                }
                case Event::Type::ShowOpticalFlow:
                {
                        const Event::show_optical_flow& d = event->get<Event::show_optical_flow>();
                        direct_show_optical_flow(d.show);
                        return true;
                }
                case Event::Type::SetVerticalSync:
                {
                        const Event::set_vertical_sync& d = event->get<Event::set_vertical_sync>();
                        direct_set_vertical_sync(d.enable);
                        return true;
                }
                case Event::Type::SetShadowZoom:
                {
                        const Event::set_shadow_zoom& d = event->get<Event::set_shadow_zoom>();
                        direct_set_shadow_zoom(d.zoom);
                        return true;
                }
                }

                error_fatal("Unknown show event type");
        }
};
