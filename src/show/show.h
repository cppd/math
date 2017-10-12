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

#pragma once

#include "obj/obj.h"
#include "window/window_prop.h"

#include <glm/vec3.hpp>
#include <memory>
#include <string>

class IShowCallback
{
protected:
        virtual ~IShowCallback() = default;

public:
        virtual void message_error_fatal(const std::string&) noexcept = 0;
        virtual void message_error_source(const std::string&, const std::string&) noexcept = 0;
        virtual void object_loaded(int) noexcept = 0;
};

class IShow
{
public:
        virtual ~IShow() = default;

        virtual void add_object(const std::shared_ptr<IObj>&, int id, int scale_id) = 0;
        virtual void delete_object(int id) = 0;
        virtual void delete_all_objects() = 0;
        virtual void show_object(int id) = 0;
        virtual void parent_resized() = 0;
        virtual void mouse_wheel(double) = 0;
        virtual void toggle_fullscreen() = 0;
        virtual void reset_view() = 0;
        virtual void set_ambient(float) = 0;
        virtual void set_diffuse(float) = 0;
        virtual void set_specular(float) = 0;
        virtual void set_clear_color(const glm::vec3&) = 0;
        virtual void set_default_color(const glm::vec3&) = 0;
        virtual void set_wireframe_color(const glm::vec3&) = 0;
        virtual void set_default_ns(float) = 0;
        virtual void show_smooth(bool) = 0;
        virtual void show_wireframe(bool) = 0;
        virtual void show_shadow(bool) = 0;
        virtual void show_materials(bool) = 0;
        virtual void show_effect(bool) = 0;
        virtual void show_dft(bool) = 0;
        virtual void set_dft_brightness(float) = 0;
        virtual void show_convex_hull_2d(bool) = 0;
        virtual void show_optical_flow(bool) = 0;
        virtual void set_vertical_sync(bool v) = 0;
        virtual void set_shadow_zoom(float v) = 0;

        virtual void get_camera_information(glm::vec3* camera_up, glm::vec3* camera_direction, glm::vec3* light_direction,
                                            glm::vec3* view_center, float* view_width) const = 0;
        virtual void get_object_size_and_position(float* object_size, glm::vec3* object_position) const = 0;
};

std::unique_ptr<IShow> create_show(IShowCallback*, WindowID win_parent, glm::vec3 clear_color, glm::vec3 default_color,
                                   glm::vec3 wireframe_color, bool with_smooth, bool with_wireframe, bool with_shadow,
                                   bool with_materials, bool with_effect, bool with_dft, bool with_convex_hull,
                                   bool with_optical_flow, float ambient, float diffuse, float specular, float dft_brightness,
                                   float default_ns, bool vertical_sync, float shadow_zoom);
