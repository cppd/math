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

#include <src/numerical/vec.h>

#include <vector>

namespace gpu::optical_flow
{
// Размеры групп потоков вычислительных шейдеров
constexpr vec2i GROUP_SIZE(16, 16);
// Минимальный размер изображения для пирамиды изображений
constexpr int BOTTOM_IMAGE_SIZE = 16;

// Параметры алгоритма для передачи в вычислительный шейдер
// Радиус окрестности точки
constexpr int RADIUS = 6;
// Максимальное количество итераций
constexpr int ITERATION_COUNT = 10;
// Если на итерации квадрат потока меньше этого значения, то выход из цикла
constexpr float STOP_MOVE_SQUARE = square(1e-3f);
// Если определитель матрицы G меньше этого значения, то считается, что нет потока
constexpr float MIN_DETERMINANT = 1;

std::vector<vec2i> pyramid_sizes(int width, int height, int min_size);

vec2i grayscale_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);
std::vector<vec2i> downsample_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);
std::vector<vec2i> sobel_groups(const vec2i& group_size, const std::vector<vec2i>& sizes);
std::vector<vec2i> flow_groups(
        const vec2i& group_size,
        const std::vector<vec2i>& sizes,
        int top_point_count_x,
        int top_point_count_y);
}
