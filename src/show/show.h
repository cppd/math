/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/color/color.h"
#include "com/vec.h"
#include "graphics/api.h"
#include "obj/obj.h"
#include "window/handle.h"

#include <memory>
#include <optional>
#include <string>

class ShowCallback
{
protected:
        virtual ~ShowCallback() = default;

public:
        virtual void message_error_fatal(const std::string&) const noexcept = 0;
        virtual void message_error_source(const std::string&, const std::string&) const noexcept = 0;
        virtual void object_loaded(int) const noexcept = 0;
};

class Show
{
public:
        virtual ~Show() = default;

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
        virtual void set_background_color(const Color&) = 0;
        virtual void set_default_color(const Color&) = 0;
        virtual void set_wireframe_color(const Color&) = 0;
        virtual void set_default_ns(double) = 0;
        virtual void show_smooth(bool) = 0;
        virtual void show_wireframe(bool) = 0;
        virtual void show_shadow(bool) = 0;
        virtual void show_fog(bool) = 0;
        virtual void show_materials(bool) = 0;
        virtual void show_fps(bool) = 0;
        virtual void show_pencil_sketch(bool) = 0;
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

struct ShowCreateInfo
{
        // std::optional используется для проверки того, что все значения заданы
        std::optional<ShowCallback*> callback;
        std::optional<WindowID> parent_window;
        std::optional<double> parent_window_ppi;
        std::optional<Color> background_color;
        std::optional<Color> default_color;
        std::optional<Color> wireframe_color;
        std::optional<bool> with_smooth;
        std::optional<bool> with_wireframe;
        std::optional<bool> with_shadow;
        std::optional<bool> with_fog;
        std::optional<bool> with_materials;
        std::optional<bool> with_fps;
        std::optional<bool> with_pencil_sketch;
        std::optional<bool> with_dft;
        std::optional<bool> with_convex_hull;
        std::optional<bool> with_optical_flow;
        std::optional<double> ambient;
        std::optional<double> diffuse;
        std::optional<double> specular;
        std::optional<double> dft_brightness;
        std::optional<Color> dft_color;
        std::optional<Color> dft_background_color;
        std::optional<double> default_ns;
        std::optional<bool> vertical_sync;
        std::optional<double> shadow_zoom;
};

std::unique_ptr<Show> create_show(GraphicsAndComputeAPI api, const ShowCreateInfo& info);
