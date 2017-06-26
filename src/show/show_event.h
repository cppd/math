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

//   Сочетание типа сообщения и набора данных этого сообщения
// с последующей обработкой с помощью switch предпочтительнее,
// чем использование виртуальных функций:
// 1)
//      При применении switch всё содержится в одном месте,
//    и поэтому это более читаемое, чем множество мелких
//    виртуальных функций.
// 2)
//      Виртуальные функции находятся в где-то в наследниках,
//    а данные для работы находятся там, где происходит обработка
//    сообщений, поэтому виртуальные функции ничего полезного
//    не могут сделать и просто вызывают функции того класса,
//    где происходит эта обработка сообщений.
// 3)
//      Если обработка сообщений происходит switch, то можно
//    пользоваться локальными переменными, а в случае виртуальных
//    функций надо все нужные данные переносить в сам класс,
//    чтобы функции из пункта 2 имели к доступ к этим данным.

#pragma once
#ifndef SHOW_EVENT_H
#define SHOW_EVENT_H

#include "com/variant.h"
#include "obj/obj.h"

#include <glm/vec3.hpp>
#include <memory>
#include <string>

class Event final
{
public:
        struct add_object final
        {
                const std::shared_ptr<IObj> obj;
                const int id;
                const int scale_id;
                add_object(const std::shared_ptr<IObj>& obj_, int id_, int scale_id_) : obj(obj_), id(id_), scale_id(scale_id_)
                {
                }
        };
        struct delete_object final
        {
                const int id;
                delete_object(int id_) : id(id_)
                {
                }
        };
        struct show_object final
        {
                const int id;
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
                const double delta;
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
                const float ambient;
                set_ambient(float v) : ambient(v)
                {
                }
        };
        struct set_diffuse final
        {
                const float diffuse;
                set_diffuse(float v) : diffuse(v)
                {
                }
        };
        struct set_specular final
        {
                const float specular;
                set_specular(float v) : specular(v)
                {
                }
        };
        struct set_clear_color final
        {
                const glm::vec3 clear_color;
                set_clear_color(const glm::vec3& v) : clear_color(v)
                {
                }
        };
        struct set_default_color final
        {
                const glm::vec3 default_color;
                set_default_color(const glm::vec3& v) : default_color(v)
                {
                }
        };
        struct set_wireframe_color final
        {
                const glm::vec3 wireframe_color;
                set_wireframe_color(const glm::vec3& v) : wireframe_color(v)
                {
                }
        };
        struct set_default_ns final
        {
                const float default_ns;
                set_default_ns(float v) : default_ns(v)
                {
                }
        };
        struct show_smooth final
        {
                const bool show;
                show_smooth(bool v) : show(v)
                {
                }
        };
        struct show_wireframe final
        {
                const bool show;
                show_wireframe(bool v) : show(v)
                {
                }
        };
        struct show_shadow final
        {
                const bool show;
                show_shadow(bool v) : show(v)
                {
                }
        };
        struct show_materials final
        {
                const bool show;
                show_materials(bool v) : show(v)
                {
                }
        };
        struct show_effect final
        {
                const bool show;
                show_effect(bool v) : show(v)
                {
                }
        };
        struct show_dft final
        {
                const bool show;
                show_dft(bool v) : show(v)
                {
                }
        };
        struct set_dft_brightness final
        {
                const float dft_brightness;
                set_dft_brightness(float v) : dft_brightness(v)
                {
                }
        };
        struct show_convex_hull_2d final
        {
                const bool show;
                show_convex_hull_2d(bool v) : show(v)
                {
                }
        };
        struct show_optical_flow final
        {
                const bool show;
                show_optical_flow(bool v) : show(v)
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
                show_optical_flow
        };

        Event() = default;

#if 0
        template <typename T>
        Event(T&& v) : m_type(event_type(in_place<T>)), m_data(std::forward<T>(v))
        {
        }
#endif

        template <typename T, typename... Args>
        Event(InPlaceT<T>, Args&&... args) : m_type(event_type(in_place<T>)), m_data(in_place<T>, std::forward<Args>(args)...)
        {
        }

        EventType get_type() const
        {
                return m_type;
        }

        template <typename D>
        const D& get() const
        {
                return m_data.get<D>();
        }

private:
        static constexpr EventType event_type(InPlaceT<add_object>)
        {
                return EventType::add_object;
        }
        static constexpr EventType event_type(InPlaceT<delete_object>)
        {
                return EventType::delete_object;
        }
        static constexpr EventType event_type(InPlaceT<show_object>)
        {
                return EventType::show_object;
        }
        static constexpr EventType event_type(InPlaceT<delete_all_objects>)
        {
                return EventType::delete_all_objects;
        }
        static constexpr EventType event_type(InPlaceT<parent_resized>)
        {
                return EventType::parent_resized;
        }
        static constexpr EventType event_type(InPlaceT<mouse_wheel>)
        {
                return EventType::mouse_wheel;
        }
        static constexpr EventType event_type(InPlaceT<toggle_fullscreen>)
        {
                return EventType::toggle_fullscreen;
        }
        static constexpr EventType event_type(InPlaceT<reset_view>)
        {
                return EventType::reset_view;
        }
        static constexpr EventType event_type(InPlaceT<set_ambient>)
        {
                return EventType::set_ambient;
        }
        static constexpr EventType event_type(InPlaceT<set_diffuse>)
        {
                return EventType::set_diffuse;
        }
        static constexpr EventType event_type(InPlaceT<set_specular>)
        {
                return EventType::set_specular;
        }
        static constexpr EventType event_type(InPlaceT<set_clear_color>)
        {
                return EventType::set_clear_color;
        }
        static constexpr EventType event_type(InPlaceT<set_default_color>)
        {
                return EventType::set_default_color;
        }
        static constexpr EventType event_type(InPlaceT<set_wireframe_color>)
        {
                return EventType::set_wireframe_color;
        }
        static constexpr EventType event_type(InPlaceT<set_default_ns>)
        {
                return EventType::set_default_ns;
        }
        static constexpr EventType event_type(InPlaceT<show_smooth>)
        {
                return EventType::show_smooth;
        }
        static constexpr EventType event_type(InPlaceT<show_wireframe>)
        {
                return EventType::show_wireframe;
        }
        static constexpr EventType event_type(InPlaceT<show_shadow>)
        {
                return EventType::show_shadow;
        }
        static constexpr EventType event_type(InPlaceT<show_materials>)
        {
                return EventType::show_materials;
        }
        static constexpr EventType event_type(InPlaceT<show_effect>)
        {
                return EventType::show_effect;
        }
        static constexpr EventType event_type(InPlaceT<show_dft>)
        {
                return EventType::show_dft;
        }
        static constexpr EventType event_type(InPlaceT<set_dft_brightness>)
        {
                return EventType::set_dft_brightness;
        }
        static constexpr EventType event_type(InPlaceT<show_convex_hull_2d>)
        {
                return EventType::show_convex_hull_2d;
        }
        static constexpr EventType event_type(InPlaceT<show_optical_flow>)
        {
                return EventType::show_optical_flow;
        }

        EventType m_type;

        SimpleVariant<add_object, delete_object, show_object, delete_all_objects, parent_resized, mouse_wheel, toggle_fullscreen,
                      reset_view, set_ambient, set_diffuse, set_specular, set_clear_color, set_default_color, set_wireframe_color,
                      set_default_ns, show_smooth, show_wireframe, show_shadow, show_materials, show_effect, show_dft,
                      set_dft_brightness, show_convex_hull_2d, show_optical_flow>
                m_data;
};

#endif
