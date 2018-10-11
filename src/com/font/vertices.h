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

#include "com/vec.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Данные вершин треугольников для непосредственной передачи в шейдеры.
// Типы данных GLSL для координат вершин ivec2 и для координат текстуры vec2.
// Для общего случая используется int_least32_t вместо int32_t.
struct TextVertex
{
        Vector<2, int_least32_t> v; // Координаты вершины в пространстве экрана.
        Vector<2, float> t; // Координаты вершины в текстуре.

        TextVertex(int v1, int v2, float t1, float t2) : v(v1, v2), t(t1, t2)
        {
        }
};

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y,
                   const std::vector<std::string>& text, std::vector<TextVertex>* vertices);

void text_vertices(const std::unordered_map<char, FontChar>& chars, int step_y, int start_x, int start_y, const std::string& text,
                   std::vector<TextVertex>* vertices);
