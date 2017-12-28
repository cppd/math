/*
Copyright (C) 2017 Topological Manifold

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

// Если расстояние до пересечения с объектом меньше этой величины, то нет пересечения

template <typename T>
inline constexpr T INTERSECTION_THRESHOLD;

template <>
inline constexpr double INTERSECTION_THRESHOLD<double> = 1e-8;

// Для скалярных произведений

template <typename T>
inline constexpr T EPSILON;

template <>
inline constexpr double EPSILON<double> = 1e-8;
