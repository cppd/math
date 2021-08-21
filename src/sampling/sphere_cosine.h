/*
Copyright (C) 2017-2021 Topological Manifold

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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.3 Sampling random variables
13.6 2D Sampling with multidimensional transformations
*/

#pragma once

#include "sphere_uniform.h"

#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <cmath>

namespace ns::sampling
{
template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> cosine_on_hemisphere(RandomEngine& random_engine)
{
        static_assert(N > 2);

        Vector<N - 1, T> v;
        T v_length_square;

        uniform_in_sphere(random_engine, v, v_length_square);

        T n = std::sqrt(1 - v_length_square);

        Vector<N, T> coordinates;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                coordinates[i] = v[i];
        }
        coordinates[N - 1] = n;

        return coordinates;
}

template <typename RandomEngine, std::size_t N, typename T>
Vector<N, T> cosine_on_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        static_assert(N > 2);

        std::array<Vector<N, T>, N - 1> orthonormal_basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> coordinates = cosine_on_hemisphere<N, T>(random_engine);

        Vector<N, T> result = coordinates[N - 1] * normal;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                result += coordinates[i] * orthonormal_basis[i];
        }

        return result;
}

template <std::size_t N, typename T>
T cosine_on_hemisphere_pdf(T n_v)
{
        if (n_v > 0)
        {
                static constexpr T R = 1 / geometry::sphere_integrate_cosine_factor_over_hemisphere<N>();
                return n_v * R;
        }
        return 0;
}
}
