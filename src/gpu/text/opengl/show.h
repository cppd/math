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

#if defined(OPENGL_FOUND)

#include "com/color/color.h"
#include "com/font/text_data.h"

#include <memory>

namespace gpu_opengl
{
struct Text
{
        virtual ~Text() = default;

        virtual void set_color(const Color& color) const = 0;
        virtual void set_window(int x, int y, int width, int height) = 0;

        virtual void draw(const TextData& text_data) = 0;
};

std::unique_ptr<Text> create_text(int size, const Color& color);
}

#endif
