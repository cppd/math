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
#include "graphics/opengl/objects.h"

#include <memory>

struct OpenGLCanvas
{
        virtual ~OpenGLCanvas() = default;

        virtual void set_fps_text_color(const Color& c) = 0;
        virtual void set_fps_text_active(bool v) = 0;

        virtual void set_pencil_effect_active(bool v) = 0;
        virtual bool pencil_effect_active() = 0;

        virtual void set_dft_active(bool v) = 0;
        virtual bool dft_active() = 0;
        virtual void set_dft_brightness(double v) = 0;
        virtual void set_dft_background_color(const Color& c) = 0;
        virtual void set_dft_color(const Color& c) = 0;

        virtual void set_convex_hull_active(bool v) = 0;

        virtual void set_optical_flow_active(bool v) = 0;

        //

        virtual void create_objects(int window_width, int window_height, const mat4& matrix,
                                    const opengl::TextureRGBA32F& color_texture, bool color_texture_is_srgb,
                                    const opengl::TextureR32I& objects, int draw_width, int draw_height, int dft_dst_x,
                                    int dft_dst_y, bool frame_buffer_is_srgb, const char* fps_text, int text_size,
                                    int text_step_y, int text_start_x, int text_start_y) = 0;

        virtual void draw() = 0;
};

std::unique_ptr<OpenGLCanvas> create_opengl_canvas();
