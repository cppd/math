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
#include "com/matrix.h"

#include <memory>

namespace gpgpu_opengl
{
struct DFTShow
{
        virtual ~DFTShow() = default;

        virtual void set_brightness(double brightness) = 0;
        virtual void set_background_color(const Color& color) = 0;
        virtual void set_color(const Color& color) = 0;
        virtual void take_image_from_framebuffer() = 0;
        virtual void draw() = 0;
};

std::unique_ptr<DFTShow> create_dft_show(int width, int height, int dst_x, int dst_y, const mat4& matrix, bool source_srgb,
                                         double brightness, const Color& background_color, const Color& color);
}
