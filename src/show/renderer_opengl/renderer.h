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
#include "com/mat.h"
#include "com/vec.h"
#include "graphics/opengl/objects.h"
#include "obj/obj.h"

#include <memory>

struct OpenGLRenderer
{
        virtual ~OpenGLRenderer() = default;

        virtual void set_light_a(const Color& light) = 0;
        virtual void set_light_d(const Color& light) = 0;
        virtual void set_light_s(const Color& light) = 0;
        virtual void set_background_color(const Color& color) = 0;
        virtual void set_default_color(const Color& color) = 0;
        virtual void set_wireframe_color(const Color& color) = 0;
        virtual void set_default_ns(double default_ns) = 0;
        virtual void set_show_smooth(bool show) = 0;
        virtual void set_show_wireframe(bool show) = 0;
        virtual void set_show_shadow(bool show) = 0;
        virtual void set_show_fog(bool show) = 0;
        virtual void set_show_materials(bool show) = 0;
        virtual void set_shadow_zoom(double zoom) = 0;
        virtual void set_matrices(const mat4& shadow_matrix, const mat4& main_matrix) = 0;
        virtual void set_light_direction(vec3 dir) = 0;
        virtual void set_camera_direction(vec3 dir) = 0;
        virtual void set_size(int width, int height) = 0;

        virtual void object_add(const Obj<3>* obj, double size, const vec3& position, int id, int scale_id) = 0;
        virtual void object_delete(int id) = 0;
        virtual void object_show(int id) = 0;
        virtual void object_delete_all() = 0;

        virtual bool draw(bool draw_to_buffer) = 0;

        virtual const TextureRGBA32F& color_buffer_texture() const = 0;
        virtual const TextureR32I& object_texture() const = 0;

        virtual bool frame_buffer_is_srgb() = 0;
        virtual bool color_buffer_is_srgb() = 0;

        static mat4 ortho(double left, double right, double bottom, double top, double near, double far);
};

std::unique_ptr<OpenGLRenderer> create_opengl_renderer();
