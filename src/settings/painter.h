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

namespace settings::painter
{
using FloatingPoint = double;

// Количество лучей на один пиксель на одно измерение в одном проходе.
// Тогда для количества измерений D в пространстве экрана количество
// лучей равно std::pow(эта_величина, D).
constexpr int DEFAULT_SAMPLES_PER_DIMENSION = 5;
constexpr int MAXIMUM_SAMPLES_PER_DIMENSION = 10;

// Максимальный размер экрана в пикселях для 3 измерений
constexpr int MAXIMUM_SCREEN_SIZE_3D = 10000;

// Размеры экрана в пикселях для 4 и более измерений
constexpr int DEFAULT_SCREEN_SIZE = 500;
constexpr int MINIMUM_SCREEN_SIZE = 50;
constexpr int MAXIMUM_SCREEN_SIZE = 5000;
}
