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

#include "vertices.h"

#include "com/error.h"
#include "com/unicode/names.h"
#include "com/unicode/unicode.h"

constexpr char32_t REPLACEMENT_CHARACTER = unicode_code_points::REPLACEMENT_CHARACTER;

namespace
{
template <typename T>
const FontChar& char_data(const std::unordered_map<T, FontChar>& chars, T code_point)
{
        static_assert(std::is_same_v<T, char32_t>);

        auto iter = chars.find(code_point);
        if (iter == chars.cend())
        {
                iter = chars.find(REPLACEMENT_CHARACTER);
                if (iter == chars.cend())
                {
                        error("Error finding character " + unicode::utf32_to_number_string(code_point) +
                              " and replacement character " + unicode::utf32_to_number_string(REPLACEMENT_CHARACTER));
                }
        }
        return iter->second;
}

void text_vertices(const std::unordered_map<char32_t, FontChar>& chars, int step_y, int start_x, int& x, int& y,
                   const std::string& text, std::vector<TextVertex>* vertices)
{
        size_t i = 0;
        while (i < text.size())
        {
                if (text[i] == '\n')
                {
                        y += step_y;
                        x = start_x;
                        ++i;
                        continue;
                }

                char32_t code_point = unicode::read_utf8_as_utf32(text, i);

                const FontChar& fc = char_data(chars, code_point);

                int x0 = x + fc.left;
                int y0 = y - fc.top;
                int x1 = x0 + fc.width;
                int y1 = y0 + fc.height;

                vertices->emplace_back(x0, y0, fc.s0, fc.t0);
                vertices->emplace_back(x1, y0, fc.s1, fc.t0);
                vertices->emplace_back(x0, y1, fc.s0, fc.t1);

                vertices->emplace_back(x1, y0, fc.s1, fc.t0);
                vertices->emplace_back(x0, y1, fc.s0, fc.t1);
                vertices->emplace_back(x1, y1, fc.s1, fc.t1);

                x += fc.advance_x;
        }
}
}

void text_vertices(const std::unordered_map<char32_t, FontChar>& chars, int step_y, int start_x, int start_y,
                   const std::vector<std::string>& text, std::vector<TextVertex>* vertices)
{
        vertices->clear();

        int x = start_x;
        int y = start_y;

        for (const std::string& s : text)
        {
                text_vertices(chars, step_y, start_x, x, y, s, vertices);
        }
}

void text_vertices(const std::unordered_map<char32_t, FontChar>& chars, int step_y, int start_x, int start_y,
                   const std::string& text, std::vector<TextVertex>* vertices)
{
        vertices->clear();

        int x = start_x;
        int y = start_y;

        text_vertices(chars, step_y, start_x, x, y, text, vertices);
}
