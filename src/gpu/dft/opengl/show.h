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
#include "graphics/opengl/buffers.h"

#include <memory>

namespace gpu_opengl
{
struct DFTShow
{
        virtual ~DFTShow() = default;

        virtual void set_brightness(double brightness) = 0;
        virtual void set_background_color(const Color& color) = 0;
        virtual void set_color(const Color& color) = 0;
        virtual void draw() = 0;
};

std::unique_ptr<DFTShow> create_dft_show(const opengl::Texture& source, int x, int y, int width, int height, double brightness,
                                         const Color& background_color, const Color& color);
}
