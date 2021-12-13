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

#pragma once

#include "brdf.h"

#include <src/com/random/engine.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_uniform.h>

#include <random>

namespace ns::shading::test
{
namespace random_implementation
{
template <std::size_t N, typename T>
std::array<Vector<N, T>, 2> random_n_v()
{
        std::mt19937 random_engine = create_engine<std::mt19937>();

        const Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);

        Vector<N, T> v;
        T d;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(random_engine);
                d = dot(n, v);
        } while (!(std::abs(d) > T(0.1)));

        if (d > 0)
        {
                return {n, v};
        }
        return {n, -v};
}
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Color directional_albedo_uniform_sampling(const BRDF<N, T, Color, RandomEngine>& brdf, const long long sample_count)
{
        const auto [n, v] = random_implementation::random_n_v<N, T>();
        return directional_albedo_uniform_sampling(brdf, n, v, sample_count);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
T directional_pdf_integral(const BRDF<N, T, Color, RandomEngine>& brdf, const long long sample_count)
{
        const auto [n, v] = random_implementation::random_n_v<N, T>();
        return directional_pdf_integral(brdf, n, v, sample_count);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Color directional_albedo_importance_sampling(const BRDF<N, T, Color, RandomEngine>& brdf, const long long sample_count)
{
        const auto [n, v] = random_implementation::random_n_v<N, T>();
        return directional_albedo_importance_sampling(brdf, n, v, sample_count);
}
}
