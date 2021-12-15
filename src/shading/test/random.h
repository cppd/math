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

template <typename Color>
Color random_non_black_color()
{
        std::mt19937 random_engine = create_engine<std::mt19937>();
        std::uniform_real_distribution<double> urd(0, 1);

        Color color;
        do
        {
                const double r = urd(random_engine);
                const double g = urd(random_engine);
                const double b = urd(random_engine);
                color = Color(r, g, b);
        } while (color.is_black());

        return color;
}
}
