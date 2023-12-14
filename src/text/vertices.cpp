/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "vertices.h"

#include "glyphs.h"
#include "text_data.h"
#include "unicode.h"

#include <src/com/error.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns::text
{
namespace
{
constexpr char32_t DEFAULT_CHARACTER = unicode::SPACE;

template <typename T>
const FontGlyph& code_point_glyph(const std::unordered_map<T, FontGlyph>& glyphs, const T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        auto iter = glyphs.find(code_point);
        if (iter == glyphs.cend())
        {
                iter = glyphs.find(DEFAULT_CHARACTER);
                if (iter == glyphs.cend())
                {
                        error("Error finding character " + unicode::utf32_to_number_string(code_point)
                              + " and default character " + unicode::utf32_to_number_string(DEFAULT_CHARACTER));
                }
        }
        return iter->second;
}

void text_vertices(
        const std::unordered_map<char32_t, FontGlyph>& glyphs,
        const int step_y,
        const int start_x,
        const std::string& text,
        int* const x,
        int* const y,
        std::vector<TextVertex>* const vertices)
{
        std::size_t i = 0;
        while (i < text.size())
        {
                if (text[i] == '\n')
                {
                        *y += step_y;
                        *x = start_x;
                        ++i;
                        continue;
                }

                const char32_t code_point = unicode::utf8_to_utf32(text, &i);

                const FontGlyph& g = code_point_glyph(glyphs, code_point);

                const int x0 = *x + g.left;
                const int y0 = *y - g.top;
                const int x1 = x0 + g.width;
                const int y1 = y0 + g.height;

                vertices->emplace_back(x0, y0, g.s0, g.t0);
                vertices->emplace_back(x1, y0, g.s1, g.t0);
                vertices->emplace_back(x0, y1, g.s0, g.t1);

                vertices->emplace_back(x1, y0, g.s1, g.t0);
                vertices->emplace_back(x0, y1, g.s0, g.t1);
                vertices->emplace_back(x1, y1, g.s1, g.t1);

                *x += g.advance_x;
        }
}
}

void text_vertices(
        const std::unordered_map<char32_t, FontGlyph>& glyphs,
        const TextData& text_data,
        std::vector<TextVertex>* const vertices)
{
        vertices->clear();

        int x = text_data.start_x;
        int y = text_data.start_y;

        for (const std::string& s : text_data.text)
        {
                text_vertices(glyphs, text_data.step_y, text_data.start_x, s, &x, &y, vertices);
        }
}
}
