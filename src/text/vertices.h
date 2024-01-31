/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "glyphs.h"
#include "text_data.h"

#include <src/numerical/vector.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ns::text
{
struct TextVertex final
{
        numerical::Vector<2, std::int_least32_t> window;
        numerical::Vector<2, float> texture;

        TextVertex(const int w1, const int w2, const float t1, const float t2)
                : window(w1, w2),
                  texture(t1, t2)
        {
        }
};

void text_vertices(
        const std::unordered_map<char32_t, FontGlyph>& glyphs,
        const TextData& text_data,
        std::vector<TextVertex>* vertices);
}
