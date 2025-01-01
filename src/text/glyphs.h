/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "font.h"

#include <src/image/image.h>

#include <unordered_map>

namespace ns::text
{
struct FontGlyph final
{
        int width;
        int height;
        int left;
        int top;
        int advance_x;

        float s0;
        float s1;
        float t0;
        float t1;
};

struct FontGlyphs final
{
        std::unordered_map<char32_t, FontGlyph> glyphs;
        image::Image<2> image;
};

FontGlyphs create_font_glyphs(const Font& font, int max_width, int max_height);
}
