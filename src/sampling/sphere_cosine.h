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
 13.6 2D Sampling with multidimensional transformations.
*/

#pragma once

#include "sphere_uniform.h"

#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <random>

namespace ns::sampling
{
template <typename RandomEngine, std::size_t N, typename T>
Vector<N, T> random_cosine_weighted_on_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        static_assert(N > 2);

        Vector<N - 1, T> v;
        T v_length_square;

        random_in_sphere(random_engine, v, v_length_square);

        T v_length = std::sqrt(v_length_square);
        T n = std::sqrt((1 - v_length) * (1 + v_length));

        std::array<Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> res = n * normal;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                res += v[i] * basis[i];
        }

        return res;
}

template <typename RandomEngine, typename T>
Vector<3, T> random_power_cosine_weighted_on_hemisphere(
        RandomEngine& random_engine,
        const Vector<3, T>& normal,
        T power)
{
        constexpr unsigned N = 3;

        // 3-space only
        //
        // angle = ∠(vector, normal)
        // PDF = cos(angle)^n * sin(angle), 0 <= angle <= PI/2
        //
        // d = Assuming[n >= 0,
        //   ProbabilityDistribution[(Cos[x]^n) * Sin[x], {x, 0, Pi/2}, Method -> "Normalize"]];
        // PDF[d, x]
        // CDF[d, x]
        //
        // CDF = 1 - cos(angle)^(1 + n)
        // Inverse CDF = acos((1 - CDF)^(1 / (1 + n)))
        // Inverse CDF = acos(x^(1 / (1 + n)))
        // Projection on normal = cos(acos(x^(1 / (1 + n))))
        // Projection on normal = x^(1 / (1 + n))
        //
        // uniform x = length_of_random_vector_in_2_sphere ^ 2
        // Projection on normal = squared_length_of_random_vector_in_2_sphere ^ (1 / (1 + n))

        Vector<N - 1, T> v;
        T v_length_square;

        random_in_sphere(random_engine, v, v_length_square);

        T n = std::pow(v_length_square, 1 / (1 + power));
        T new_length_squared = (1 - n) * (1 + n);
        v *= std::sqrt(new_length_squared / v_length_square);

        std::array<Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> res = n * normal;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                res += v[i] * basis[i];
        }

        return res;
}
}
