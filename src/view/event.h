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

#include <src/color/color.h>
#include <src/model/mesh_object.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>
#include <src/window/handle.h>

#include <variant>

namespace view
{
enum class MouseButton
{
        Left,
        Right
};

struct Event final
{
        struct add_object_t final
        {
                std::shared_ptr<const mesh::MeshObject<3>> object;
                explicit add_object_t(const std::shared_ptr<const mesh::MeshObject<3>>& object_) : object(object_)
                {
                }
        };
        struct delete_object_t final
        {
                ObjectId id;
                explicit delete_object_t(ObjectId id_) : id(id_)
                {
                }
        };
        struct show_object_t final
        {
                ObjectId id;
                explicit show_object_t(ObjectId id_) : id(id_)
                {
                }
        };
        struct delete_all_objects_t final
        {
        };
        struct reset_view_t final
        {
        };
        struct set_ambient_t final
        {
                double ambient;
                explicit set_ambient_t(double v) : ambient(v)
                {
                }
        };
        struct set_diffuse_t final
        {
                double diffuse;
                explicit set_diffuse_t(double v) : diffuse(v)
                {
                }
        };
        struct set_specular_t final
        {
                double specular;
                explicit set_specular_t(double v) : specular(v)
                {
                }
        };
        struct set_background_color_t final
        {
                Color background_color;
                explicit set_background_color_t(const Color& v) : background_color(v)
                {
                }
        };
        struct set_default_color_t final
        {
                Color default_color;
                explicit set_default_color_t(const Color& v) : default_color(v)
                {
                }
        };
        struct set_wireframe_color_t final
        {
                Color wireframe_color;
                explicit set_wireframe_color_t(const Color& v) : wireframe_color(v)
                {
                }
        };
        struct set_clip_plane_color_t final
        {
                Color clip_plane_color;
                explicit set_clip_plane_color_t(const Color& v) : clip_plane_color(v)
                {
                }
        };
        struct set_normal_length_t final
        {
                float length;
                explicit set_normal_length_t(float v) : length(v)
                {
                }
        };
        struct set_normal_color_positive_t final
        {
                Color color;
                explicit set_normal_color_positive_t(const Color& v) : color(v)
                {
                }
        };
        struct set_normal_color_negative_t final
        {
                Color color;
                explicit set_normal_color_negative_t(const Color& v) : color(v)
                {
                }
        };
        struct set_default_ns_t final
        {
                double default_ns;
                explicit set_default_ns_t(double v) : default_ns(v)
                {
                }
        };
        struct show_smooth_t final
        {
                bool show;
                explicit show_smooth_t(bool v) : show(v)
                {
                }
        };
        struct show_wireframe_t final
        {
                bool show;
                explicit show_wireframe_t(bool v) : show(v)
                {
                }
        };
        struct show_shadow_t final
        {
                bool show;
                explicit show_shadow_t(bool v) : show(v)
                {
                }
        };
        struct show_fog_t final
        {
                bool show;
                explicit show_fog_t(bool v) : show(v)
                {
                }
        };
        struct show_materials_t final
        {
                bool show;
                explicit show_materials_t(bool v) : show(v)
                {
                }
        };
        struct show_fps_t final
        {
                bool show;
                explicit show_fps_t(bool v) : show(v)
                {
                }
        };
        struct show_pencil_sketch_t final
        {
                bool show;
                explicit show_pencil_sketch_t(bool v) : show(v)
                {
                }
        };
        struct show_dft_t final
        {
                bool show;
                explicit show_dft_t(bool v) : show(v)
                {
                }
        };
        struct set_dft_brightness_t final
        {
                double dft_brightness;
                explicit set_dft_brightness_t(double v) : dft_brightness(v)
                {
                }
        };
        struct set_dft_background_color_t final
        {
                Color color;
                explicit set_dft_background_color_t(const Color& c) : color(c)
                {
                }
        };
        struct set_dft_color_t final
        {
                Color color;
                explicit set_dft_color_t(const Color& c) : color(c)
                {
                }
        };
        struct show_convex_hull_2d_t final
        {
                bool show;
                explicit show_convex_hull_2d_t(bool v) : show(v)
                {
                }
        };
        struct show_optical_flow_t final
        {
                bool show;
                explicit show_optical_flow_t(bool v) : show(v)
                {
                }
        };
        struct set_vertical_sync_t final
        {
                bool enable;
                explicit set_vertical_sync_t(bool v) : enable(v)
                {
                }
        };
        struct set_shadow_zoom_t final
        {
                double zoom;
                explicit set_shadow_zoom_t(double v) : zoom(v)
                {
                }
        };
        struct clip_plane_show_t final
        {
                double position;
                explicit clip_plane_show_t(double p) : position(p)
                {
                }
        };
        struct clip_plane_position_t final
        {
                double position;
                explicit clip_plane_position_t(double p) : position(p)
                {
                }
        };
        struct clip_plane_hide_t final
        {
        };
        struct show_normals_t final
        {
                bool show;
                explicit show_normals_t(bool v) : show(v)
                {
                }
        };
        struct mouse_press_t final
        {
                int x, y;
                MouseButton button;
                mouse_press_t(int x_coord, int y_coord, MouseButton b) : x(x_coord), y(y_coord), button(b)
                {
                }
        };
        struct mouse_release_t final
        {
                int x, y;
                MouseButton button;
                mouse_release_t(int x_coord, int y_coord, MouseButton b) : x(x_coord), y(y_coord), button(b)
                {
                }
        };
        struct mouse_move_t final
        {
                int x, y;
                mouse_move_t(int x_coord, int y_coord) : x(x_coord), y(y_coord)
                {
                }
        };
        struct mouse_wheel_t final
        {
                int x, y;
                double delta;
                mouse_wheel_t(int x_coord, int y_coord, double d) : x(x_coord), y(y_coord), delta(d)
                {
                }
        };
        struct window_resize_t final
        {
                int x, y;
                window_resize_t(int x_coord, int y_coord) : x(x_coord), y(y_coord)
                {
                }
        };

        using EventType = std::variant<
                add_object_t,
                delete_all_objects_t,
                delete_object_t,
                reset_view_t,
                set_ambient_t,
                set_background_color_t,
                set_default_color_t,
                set_default_ns_t,
                set_dft_background_color_t,
                set_dft_brightness_t,
                set_dft_color_t,
                set_diffuse_t,
                set_shadow_zoom_t,
                set_specular_t,
                set_vertical_sync_t,
                set_wireframe_color_t,
                set_clip_plane_color_t,
                set_normal_length_t,
                set_normal_color_positive_t,
                set_normal_color_negative_t,
                show_convex_hull_2d_t,
                show_dft_t,
                show_fps_t,
                show_pencil_sketch_t,
                show_fog_t,
                show_materials_t,
                show_object_t,
                show_optical_flow_t,
                show_shadow_t,
                show_smooth_t,
                show_wireframe_t,
                clip_plane_show_t,
                clip_plane_position_t,
                clip_plane_hide_t,
                show_normals_t,
                mouse_press_t,
                mouse_release_t,
                mouse_move_t,
                mouse_wheel_t,
                window_resize_t>;

        template <typename... Args>
        explicit Event(Args&&... args) : m_event(std::forward<Args>(args)...)
        {
        }

        const EventType& event() const
        {
                return m_event;
        }

        static Event add_object(const std::shared_ptr<const mesh::MeshObject<3>>& object)
        {
                return Event(std::in_place_type<Event::add_object_t>, object);
        }
        static Event delete_object(ObjectId id)
        {
                return Event(std::in_place_type<Event::delete_object_t>, id);
        }
        static Event show_object(ObjectId id)
        {
                return Event(std::in_place_type<Event::show_object_t>, id);
        }
        static Event delete_all_objects()
        {
                return Event(std::in_place_type<Event::delete_all_objects_t>);
        }
        static Event reset_view()
        {
                return Event(std::in_place_type<Event::reset_view_t>);
        }
        static Event set_ambient(double v)
        {
                return Event(std::in_place_type<Event::set_ambient_t>, v);
        }
        static Event set_diffuse(double v)
        {
                return Event(std::in_place_type<Event::set_diffuse_t>, v);
        }
        static Event set_specular(double v)
        {
                return Event(std::in_place_type<Event::set_specular_t>, v);
        }
        static Event set_background_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_background_color_t>, c);
        }
        static Event set_default_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_default_color_t>, c);
        }
        static Event set_wireframe_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_wireframe_color_t>, c);
        }
        static Event set_clip_plane_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_clip_plane_color_t>, c);
        }
        static Event set_normal_length(float v)
        {
                return Event(std::in_place_type<Event::set_normal_length_t>, v);
        }
        static Event set_normal_color_positive(const Color& c)
        {
                return Event(std::in_place_type<Event::set_normal_color_positive_t>, c);
        }
        static Event set_normal_color_negative(const Color& c)
        {
                return Event(std::in_place_type<Event::set_normal_color_negative_t>, c);
        }
        static Event set_default_ns(double ns)
        {
                return Event(std::in_place_type<Event::set_default_ns_t>, ns);
        }
        static Event show_smooth(bool v)
        {
                return Event(std::in_place_type<Event::show_smooth_t>, v);
        }
        static Event show_wireframe(bool v)
        {
                return Event(std::in_place_type<Event::show_wireframe_t>, v);
        }
        static Event show_shadow(bool v)
        {
                return Event(std::in_place_type<Event::show_shadow_t>, v);
        }
        static Event show_fog(bool v)
        {
                return Event(std::in_place_type<Event::show_fog_t>, v);
        }
        static Event show_materials(bool v)
        {
                return Event(std::in_place_type<Event::show_materials_t>, v);
        }
        static Event show_fps(bool v)
        {
                return Event(std::in_place_type<Event::show_fps_t>, v);
        }
        static Event show_pencil_sketch(bool v)
        {
                return Event(std::in_place_type<Event::show_pencil_sketch_t>, v);
        }
        static Event show_dft(bool v)
        {
                return Event(std::in_place_type<Event::show_dft_t>, v);
        }
        static Event set_dft_brightness(double v)
        {
                return Event(std::in_place_type<Event::set_dft_brightness_t>, v);
        }
        static Event set_dft_background_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_dft_background_color_t>, c);
        }
        static Event set_dft_color(const Color& c)
        {
                return Event(std::in_place_type<Event::set_dft_color_t>, c);
        }
        static Event show_convex_hull_2d(bool v)
        {
                return Event(std::in_place_type<Event::show_convex_hull_2d_t>, v);
        }
        static Event show_optical_flow(bool v)
        {
                return Event(std::in_place_type<Event::show_optical_flow_t>, v);
        }
        static Event set_vertical_sync(bool v)
        {
                return Event(std::in_place_type<Event::set_vertical_sync_t>, v);
        }
        static Event set_shadow_zoom(double v)
        {
                return Event(std::in_place_type<Event::set_shadow_zoom_t>, v);
        }
        static Event clip_plane_show(double v)
        {
                return Event(std::in_place_type<Event::clip_plane_show_t>, v);
        }
        static Event clip_plane_position(double v)
        {
                return Event(std::in_place_type<Event::clip_plane_position_t>, v);
        }
        static Event clip_plane_hide()
        {
                return Event(std::in_place_type<Event::clip_plane_hide_t>);
        }
        static Event show_normals(bool v)
        {
                return Event(std::in_place_type<Event::show_normals_t>, v);
        }
        static Event mouse_press(int x, int y, MouseButton button)
        {
                return Event(std::in_place_type<Event::mouse_press_t>, x, y, button);
        }
        static Event mouse_release(int x, int y, MouseButton button)
        {
                return Event(std::in_place_type<Event::mouse_release_t>, x, y, button);
        }
        static Event mouse_move(int x, int y)
        {
                return Event(std::in_place_type<Event::mouse_move_t>, x, y);
        }
        static Event mouse_wheel(int x, int y, double delta)
        {
                return Event(std::in_place_type<Event::mouse_wheel_t>, x, y, delta);
        }
        static Event window_resize(int x, int y)
        {
                return Event(std::in_place_type<Event::window_resize_t>, x, y);
        }

private:
        EventType m_event;
};

struct CameraInfo final
{
        vec3 camera_up;
        vec3 camera_direction;
        vec3 light_direction;
        vec3 view_center;
        double view_width;
        int width;
        int height;
};

struct Info final
{
        struct camera_information_t final
        {
                CameraInfo camera_info;
        };
        struct object_size_t final
        {
                double size;
        };
        struct object_position_t final
        {
                vec3 position;
        };

        using EventType = std::variant<camera_information_t, object_size_t, object_position_t>;

        template <typename... Args>
        explicit Info(Args&&... args) : m_event(std::forward<Args>(args)...)
        {
        }

        EventType& event()
        {
                return m_event;
        }

        const CameraInfo& as_camera_information() const
        {
                return std::get<Info::camera_information_t>(m_event).camera_info;
        }
        const double& as_object_size() const
        {
                return std::get<Info::object_size_t>(m_event).size;
        }
        const vec3& as_object_position() const
        {
                return std::get<Info::object_position_t>(m_event).position;
        }

        static Info camera_information()
        {
                return Info(std::in_place_type<Info::camera_information_t>);
        }
        static Info object_size()
        {
                return Info(std::in_place_type<Info::object_size_t>);
        }
        static Info object_position()
        {
                return Info(std::in_place_type<Info::object_position_t>);
        }

private:
        EventType m_event;
};
}
