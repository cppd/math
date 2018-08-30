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
#include "com/vec.h"
#include "graphics/api.h"
#include "obj/obj.h"
#include "window/window_handle.h"

#include <memory>
#include <string>

class IShowCallback
{
protected:
        virtual ~IShowCallback() = default;

public:
        virtual void message_error_fatal(const std::string&) const noexcept = 0;
        virtual void message_error_source(const std::string&, const std::string&) const noexcept = 0;
        virtual void object_loaded(int) const noexcept = 0;
};

class IShow
{
public:
        virtual ~IShow() = default;

        virtual void add_object(const std::shared_ptr<const Obj<3>>&, int id, int scale_id) = 0;
        virtual void delete_object(int id) = 0;
        virtual void delete_all_objects() = 0;
        virtual void show_object(int id) = 0;
        virtual void parent_resized() = 0;
        virtual void mouse_wheel(double) = 0;
        virtual void toggle_fullscreen() = 0;
        virtual void reset_view() = 0;
        virtual void set_ambient(double) = 0;
        virtual void set_diffuse(double) = 0;
        virtual void set_specular(double) = 0;
        virtual void set_background_color_rgb(const Color&) = 0;
        virtual void set_default_color_rgb(const Color&) = 0;
        virtual void set_wireframe_color_rgb(const Color&) = 0;
        virtual void set_default_ns(double) = 0;
        virtual void show_smooth(bool) = 0;
        virtual void show_wireframe(bool) = 0;
        virtual void show_shadow(bool) = 0;
        virtual void show_fog(bool) = 0;
        virtual void show_materials(bool) = 0;
        virtual void show_effect(bool) = 0;
        virtual void show_dft(bool) = 0;
        virtual void set_dft_brightness(double) = 0;
        virtual void set_dft_background_color(const Color&) = 0;
        virtual void set_dft_color(const Color&) = 0;
        virtual void show_convex_hull_2d(bool) = 0;
        virtual void show_optical_flow(bool) = 0;
        virtual void set_vertical_sync(bool v) = 0;
        virtual void set_shadow_zoom(double v) = 0;

        virtual void camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width,
                                        int* paint_width, int* paint_height) const = 0;
        virtual vec3 light_direction() const = 0;
        virtual double object_size() const = 0;
        virtual vec3 object_position() const = 0;
};

std::unique_ptr<IShow> create_show(GraphicsAndComputeAPI api, IShowCallback* callback, WindowID parent_window,
                                   double parent_window_dpi, const Color& background_color_rgb, const Color& default_color_rgb,
                                   const Color& wireframe_color_rgb, bool with_smooth, bool with_wireframe, bool with_shadow,
                                   bool with_fog, bool with_materials, bool with_effect, bool with_dft, bool with_convex_hull,
                                   bool with_optical_flow, double ambient, double diffuse, double specular, double dft_brightness,
                                   const Color& dft_color, const Color& dft_background_color, double default_ns,
                                   bool vertical_sync, double shadow_zoom);
