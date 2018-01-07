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

#include "com/vec.h"
#include "obj/obj.h"

#include <memory>
#include <string>

#if !defined(STD_VARIANT_NOT_FOUND)
#include <variant>
#else
#include "com/simple_variant.h"
#endif

class Event final
{
public:
        struct add_object final
        {
                std::shared_ptr<IObj> obj;
                int id;
                int scale_id;
                add_object(const std::shared_ptr<IObj>& obj_, int id_, int scale_id_) : obj(obj_), id(id_), scale_id(scale_id_)
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
        struct set_clear_color final
        {
                vec3 clear_color;
                set_clear_color(const vec3& v) : clear_color(v)
                {
                }
        };
        struct set_default_color final
        {
                vec3 default_color;
                set_default_color(const vec3& v) : default_color(v)
                {
                }
        };
        struct set_wireframe_color final
        {
                vec3 wireframe_color;
                set_wireframe_color(const vec3& v) : wireframe_color(v)
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
        struct show_materials final
        {
                bool show;
                show_materials(bool v) : show(v)
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
        struct vertical_sync final
        {
                bool enable;
                vertical_sync(bool v) : enable(v)
                {
                }
        };
        struct shadow_zoom final
        {
                double zoom;
                shadow_zoom(double v) : zoom(v)
                {
                }
        };

        enum class EventType
        {
                add_object,
                delete_object,
                show_object,
                delete_all_objects,
                parent_resized,
                mouse_wheel,
                toggle_fullscreen,
                reset_view,
                set_ambient,
                set_diffuse,
                set_specular,
                set_clear_color,
                set_default_color,
                set_wireframe_color,
                set_default_ns,
                show_smooth,
                show_wireframe,
                show_shadow,
                show_materials,
                show_effect,
                show_dft,
                set_dft_brightness,
                show_convex_hull_2d,
                show_optical_flow,
                vertical_sync,
                shadow_zoom
        };

#if 0
        template <typename T>
        Event(T&& v) : m_type(event_type(in_place<T>)), m_data(std::forward<T>(v))
        {
        }
#endif

        template <typename T, typename... Args>
        Event(std::in_place_type_t<T>, Args&&... args)
                : m_type(event_type(std::in_place_type<T>)), m_data(std::in_place_type<T>, std::forward<Args>(args)...)
        {
        }

        EventType get_type() const
        {
                return m_type;
        }

        template <typename D>
        const D& get() const
        {
#if !defined(STD_VARIANT_NOT_FOUND)
                return std::get<D>(m_data);
#else
                return m_data.get<D>();
#endif
        }

private:
        static constexpr EventType event_type(std::in_place_type_t<add_object>)
        {
                return EventType::add_object;
        }
        static constexpr EventType event_type(std::in_place_type_t<delete_object>)
        {
                return EventType::delete_object;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_object>)
        {
                return EventType::show_object;
        }
        static constexpr EventType event_type(std::in_place_type_t<delete_all_objects>)
        {
                return EventType::delete_all_objects;
        }
        static constexpr EventType event_type(std::in_place_type_t<parent_resized>)
        {
                return EventType::parent_resized;
        }
        static constexpr EventType event_type(std::in_place_type_t<mouse_wheel>)
        {
                return EventType::mouse_wheel;
        }
        static constexpr EventType event_type(std::in_place_type_t<toggle_fullscreen>)
        {
                return EventType::toggle_fullscreen;
        }
        static constexpr EventType event_type(std::in_place_type_t<reset_view>)
        {
                return EventType::reset_view;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_ambient>)
        {
                return EventType::set_ambient;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_diffuse>)
        {
                return EventType::set_diffuse;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_specular>)
        {
                return EventType::set_specular;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_clear_color>)
        {
                return EventType::set_clear_color;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_default_color>)
        {
                return EventType::set_default_color;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_wireframe_color>)
        {
                return EventType::set_wireframe_color;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_default_ns>)
        {
                return EventType::set_default_ns;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_smooth>)
        {
                return EventType::show_smooth;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_wireframe>)
        {
                return EventType::show_wireframe;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_shadow>)
        {
                return EventType::show_shadow;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_materials>)
        {
                return EventType::show_materials;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_effect>)
        {
                return EventType::show_effect;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_dft>)
        {
                return EventType::show_dft;
        }
        static constexpr EventType event_type(std::in_place_type_t<set_dft_brightness>)
        {
                return EventType::set_dft_brightness;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_convex_hull_2d>)
        {
                return EventType::show_convex_hull_2d;
        }
        static constexpr EventType event_type(std::in_place_type_t<show_optical_flow>)
        {
                return EventType::show_optical_flow;
        }
        static constexpr EventType event_type(std::in_place_type_t<vertical_sync>)
        {
                return EventType::vertical_sync;
        }
        static constexpr EventType event_type(std::in_place_type_t<shadow_zoom>)
        {
                return EventType::shadow_zoom;
        }

        EventType m_type;

#if !defined(STD_VARIANT_NOT_FOUND)
        std::variant
#else
        SimpleVariant
#endif
                <add_object, delete_object, show_object, delete_all_objects, parent_resized, mouse_wheel, toggle_fullscreen,
                 reset_view, set_ambient, set_diffuse, set_specular, set_clear_color, set_default_color, set_wireframe_color,
                 set_default_ns, show_smooth, show_wireframe, show_shadow, show_materials, show_effect, show_dft,
                 set_dft_brightness, show_convex_hull_2d, show_optical_flow, vertical_sync, shadow_zoom>
                        m_data;
};
