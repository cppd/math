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
#include "com/print.h"

constexpr char DEFAULT_CHAR = ' ';

namespace
{
template <typename T>
constexpr unsigned char_to_int(T c)
{
        static_assert(std::is_same_v<T, char>);
        return static_cast<unsigned char>(c);
}

const FontChar& char_data(const std::unordered_map<char, FontChar>& chars, char c, char default_char)
{
        auto iter = chars.find(c);
        if (iter == chars.cend())
        {
                iter = chars.find(default_char);
                if (iter == chars.cend())
                {
                        error("Error finding character " + to_string(char_to_int(c)) + " and default character " +
                              to_string(char_to_int(default_char)));
                }
        }
        return iter->second;
}

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int& x, int& y,
                   const std::string& text, std::vector<TextVertex>* vertices)
{
        for (char c : text)
        {
                if (c == '\n')
                {
                        y += step_y;
                        x = start_x;
                        continue;
                }

                const FontChar& fc = char_data(chars, c, DEFAULT_CHAR);

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

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y,
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

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y, const std::string& text,
                   std::vector<TextVertex>* vertices)
{
        vertices->clear();

        int x = start_x;
        int y = start_y;

        text_vertices(chars, step_y, start_x, x, y, text, vertices);
}
