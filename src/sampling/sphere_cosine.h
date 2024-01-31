/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>

namespace ns::sampling
{
template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> cosine_on_hemisphere(RandomEngine& engine)
{
        static_assert(N > 2);

        numerical::Vector<N - 1, T> v;
        T v_length_square;

        uniform_in_sphere(engine, v, v_length_square);

        const T n = std::sqrt(1 - v_length_square);

        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res[i] = v[i];
        }
        res[N - 1] = n;

        return res;
}

template <typename RandomEngine, std::size_t N, typename T>
numerical::Vector<N, T> cosine_on_hemisphere(RandomEngine& engine, const numerical::Vector<N, T>& normal)
{
        static_assert(N > 2);

        std::array<numerical::Vector<N, T>, N - 1> orthonormal_basis =
                numerical::orthogonal_complement_of_unit_vector(normal);

        const numerical::Vector<N, T> coordinates = cosine_on_hemisphere<N, T>(engine);

        numerical::Vector<N, T> res = coordinates[N - 1] * normal;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res.multiply_add(coordinates[i], orthonormal_basis[i]);
        }

        return res;
}

template <std::size_t N, typename T>
T cosine_on_hemisphere_pdf(const T& n_v)
{
        if (n_v > 0)
        {
                static constexpr T R =
                        1 / geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;
                return n_v * R;
        }
        return 0;
}
}
