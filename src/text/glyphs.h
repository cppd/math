/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/color/color.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace text
{
struct FontGlyph
{
        int width, height, left, top, advance_x;

        // Начальные (s0, t0) и конечные (s1, t1) координаты символа в текстуре
        float s0, s1, t0, t1;
};

// Информация о символах шрифта с картинкой всех символов вместе.
// Grayscale, sRGB, uint8.
void create_font_glyphs(
        const Font& font,
        int max_width,
        int max_height,
        std::unordered_map<char32_t, FontGlyph>* font_glyphs,
        int* texture_width,
        int* texture_height,
        ColorFormat* color_format,
        std::vector<std::byte>* pixels);
}
