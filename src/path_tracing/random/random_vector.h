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

#include "com/vec.h"

#include <random>
#include <utility>

namespace RandomVectorImplementation
{
template <typename T, typename RandomEngine, typename Distribution, size_t... I>
Vector<sizeof...(I), T> random_vector(RandomEngine& engine, Distribution& distribution, std::integer_sequence<size_t, I...>)
{
        return Vector<sizeof...(I), T>((static_cast<void>(I), distribution(engine))...);
}
}

template <size_t N, typename T, typename RandomEngine, typename Distribution>
Vector<N, T> random_vector(RandomEngine& engine, Distribution& distribution)
{
        return RandomVectorImplementation::random_vector<T>(engine, distribution, std::make_integer_sequence<size_t, N>());
}

vec3 random_hemisphere_any_length(std::mt19937_64& engine, const vec3& normal);
vec3 random_sphere_any_length(std::mt19937_64& engine);
vec2 random_disk_any_length(std::mt19937_64& engine);
vec3 random_hemisphere_cosine_any_length(std::mt19937_64& engine, const vec3& normal);
