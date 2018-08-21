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

#include "com/color/colors.h"
#include "com/variant.h"
#include "com/vec.h"
#include "obj/obj.h"

#include <memory>
#include <string>

class Event final
{
public:
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
        struct set_background_color final
        {
                Color background_color;
                set_background_color(const Color& v) : background_color(v)
                {
                }
        };
        struct set_default_color final
        {
                Color default_color;
                set_default_color(const Color& v) : default_color(v)
                {
                }
        };
        struct set_wireframe_color final
        {
                Color wireframe_color;
                set_wireframe_color(const Color& v) : wireframe_color(v)
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

        enum class Type
        {
                AddObject,
                DeleteAllObjects,
                DeleteObject,
                MouseWheel,
                ParentResized,
                ResetView,
                SetAmbient,
                SetBackgroundColor,
                SetDefaultColor,
                SetDefaultNs,
                SetDftBackgroundColor,
                SetDftBrightness,
                SetDftColor,
                SetDiffuse,
                SetSpecular,
                SetWireframeColor,
                ShadowZoom,
                ShowConvexHull2d,
                ShowDft,
                ShowEffect,
                ShowFog,
                ShowMaterials,
                ShowObject,
                ShowOpticalFlow,
                ShowShadow,
                ShowSmooth,
                ShowWireframe,
                ToggleFullscreen,
                VerticalSync
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

        Type type() const
        {
                return m_type;
        }

        template <typename D>
        const D& get() const
        {
                return ::get<D>(m_data);
        }

private:
        static constexpr Type event_type(std::in_place_type_t<add_object>)
        {
                return Type::AddObject;
        }
        static constexpr Type event_type(std::in_place_type_t<delete_object>)
        {
                return Type::DeleteObject;
        }
        static constexpr Type event_type(std::in_place_type_t<show_object>)
        {
                return Type::ShowObject;
        }
        static constexpr Type event_type(std::in_place_type_t<delete_all_objects>)
        {
                return Type::DeleteAllObjects;
        }
        static constexpr Type event_type(std::in_place_type_t<parent_resized>)
        {
                return Type::ParentResized;
        }
        static constexpr Type event_type(std::in_place_type_t<mouse_wheel>)
        {
                return Type::MouseWheel;
        }
        static constexpr Type event_type(std::in_place_type_t<toggle_fullscreen>)
        {
                return Type::ToggleFullscreen;
        }
        static constexpr Type event_type(std::in_place_type_t<reset_view>)
        {
                return Type::ResetView;
        }
        static constexpr Type event_type(std::in_place_type_t<set_ambient>)
        {
                return Type::SetAmbient;
        }
        static constexpr Type event_type(std::in_place_type_t<set_diffuse>)
        {
                return Type::SetDiffuse;
        }
        static constexpr Type event_type(std::in_place_type_t<set_specular>)
        {
                return Type::SetSpecular;
        }
        static constexpr Type event_type(std::in_place_type_t<set_background_color>)
        {
                return Type::SetBackgroundColor;
        }
        static constexpr Type event_type(std::in_place_type_t<set_default_color>)
        {
                return Type::SetDefaultColor;
        }
        static constexpr Type event_type(std::in_place_type_t<set_wireframe_color>)
        {
                return Type::SetWireframeColor;
        }
        static constexpr Type event_type(std::in_place_type_t<set_default_ns>)
        {
                return Type::SetDefaultNs;
        }
        static constexpr Type event_type(std::in_place_type_t<show_smooth>)
        {
                return Type::ShowSmooth;
        }
        static constexpr Type event_type(std::in_place_type_t<show_wireframe>)
        {
                return Type::ShowWireframe;
        }
        static constexpr Type event_type(std::in_place_type_t<show_shadow>)
        {
                return Type::ShowShadow;
        }
        static constexpr Type event_type(std::in_place_type_t<show_fog>)
        {
                return Type::ShowFog;
        }
        static constexpr Type event_type(std::in_place_type_t<show_materials>)
        {
                return Type::ShowMaterials;
        }
        static constexpr Type event_type(std::in_place_type_t<show_effect>)
        {
                return Type::ShowEffect;
        }
        static constexpr Type event_type(std::in_place_type_t<show_dft>)
        {
                return Type::ShowDft;
        }
        static constexpr Type event_type(std::in_place_type_t<set_dft_brightness>)
        {
                return Type::SetDftBrightness;
        }
        static constexpr Type event_type(std::in_place_type_t<set_dft_background_color>)
        {
                return Type::SetDftBackgroundColor;
        }
        static constexpr Type event_type(std::in_place_type_t<set_dft_color>)
        {
                return Type::SetDftColor;
        }
        static constexpr Type event_type(std::in_place_type_t<show_convex_hull_2d>)
        {
                return Type::ShowConvexHull2d;
        }
        static constexpr Type event_type(std::in_place_type_t<show_optical_flow>)
        {
                return Type::ShowOpticalFlow;
        }
        static constexpr Type event_type(std::in_place_type_t<vertical_sync>)
        {
                return Type::VerticalSync;
        }
        static constexpr Type event_type(std::in_place_type_t<shadow_zoom>)
        {
                return Type::ShadowZoom;
        }

        Type m_type;

        Variant<add_object, delete_object, show_object, delete_all_objects, parent_resized, mouse_wheel, toggle_fullscreen,
                reset_view, set_ambient, set_diffuse, set_specular, set_background_color, set_default_color, set_wireframe_color,
                set_default_ns, show_smooth, show_wireframe, show_shadow, show_fog, show_materials, show_effect, show_dft,
                set_dft_brightness, set_dft_background_color, set_dft_color, show_convex_hull_2d, show_optical_flow,
                vertical_sync, shadow_zoom>
                m_data;
};
