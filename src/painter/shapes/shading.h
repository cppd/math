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
#include <src/random/sphere.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> surface_ray_direction(const Vector<N, T>& normal, RandomEngine& engine)
{
        // Распределение случайного луча с вероятностью по косинусу
        // угла между нормалью и случайным вектором.
        return random::random_cosine_weighted_on_hemisphere(engine, normal);
}

template <std::size_t N, typename T>
T surface_lighting(const Vector<N, T>& normal, const Vector<N, T>& /*dir_to_point*/, const Vector<N, T>& dir_to_light)
{
        return dot(normal, dir_to_light);
}
}
