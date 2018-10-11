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

#include "chars.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct TextVertex
{
        int_least32_t w1, w2; // Координаты вершины в пространстве экрана.
        float t1, t2; // Координаты вершины в текстуре.

        TextVertex(int_least32_t w1_, int_least32_t w2_, float t1_, float t2_) : w1(w1_), w2(w2_), t1(t1_), t2(t2_)
        {
        }
};

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y,
                   const std::vector<std::string>& text, std::vector<TextVertex>* vertices);

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y, const std::string& text,
                   std::vector<TextVertex>* vertices);
