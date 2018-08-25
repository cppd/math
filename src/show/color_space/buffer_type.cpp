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

// get_framebuffer_srgb() (glGetNamedFramebufferAttachmentParameteriv)
// возвращает неправильные значения. Поэтому нужно записать цвет в буфер
// и прочитать обратно.

#include "buffer_type.h"

#include "graphics/opengl/objects.h"

#include <cmath>
#include <string>
#include <vector>

constexpr float EPSILON = 0.01;

namespace
{
float read_color_from_buffer(float color) noexcept
{
        float c[4] = {color, color, color, 1};
        glClearBufferfv(GL_COLOR, 0, c);

        TextureRGBA32F pixel_texture(1, 1);
        pixel_texture.copy_texture_sub_image();

        std::array<GLfloat, 4> pixel;
        pixel_texture.get_texture_sub_image(0, 0, 1, 1, &pixel);

        return pixel[0];
}

void check_color(const char* buffer_name, float rgb_color, float srgb_color, float color)
{
        if (!is_finite(color) || (std::abs(color - rgb_color) >= EPSILON && std::abs(color - srgb_color) >= EPSILON))
        {
                error("Buffer color space detection failed. RGB color " + to_string(rgb_color) + " from " + buffer_name + " is " +
                      to_string(color) + ".");
        }
}
}

bool frame_buffer_is_srgb()
{
        float rgb_color = 0.5;
        float srgb_color = 0.73725;

        float color = read_color_from_buffer(rgb_color);

        check_color("the framebuffer", rgb_color, srgb_color, color);

        return std::abs(color - srgb_color) < EPSILON;
}

bool color_buffer_is_srgb()
{
        float rgb_color = 0.1;
        float srgb_color = 0.34902;

        ColorBuffer color_buffer(1, 1);

        color_buffer.bind_buffer();
        float color = read_color_from_buffer(rgb_color);
        color_buffer.unbind_buffer();

        check_color("a colorbuffer", rgb_color, srgb_color, color);

        return std::abs(color - srgb_color) < EPSILON;
}
