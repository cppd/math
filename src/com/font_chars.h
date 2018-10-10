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

#include "font.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct FontChar
{
        int width, height, left, top, advance_x;
        float texture_x, texture_y, texture_width, texture_height;
};

// Информация о символах шрифта с картинкой всех символов вместе.
// Grayscale, sRGB, uint8.
void create_font_chars(Font& font, unsigned max_width, unsigned max_height, std::unordered_map<char, FontChar>* chars,
                       int* texture_width, int* texture_height, std::vector<std::uint_least8_t>* texture_pixels);
