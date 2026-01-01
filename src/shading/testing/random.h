/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cstddef>
#include <random>

namespace ns::shading::testing
{
template <std::size_t N, typename T, typename RandomEngine>
std::array<numerical::Vector<N, T>, 2> random_n_v(RandomEngine& engine)
{
        const numerical::Vector<N, T> n = sampling::uniform_on_sphere<N, T>(engine);

        numerical::Vector<N, T> v;
        T d;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(engine);
                d = dot(n, v);
        } while (!(std::abs(d) > T{0.1}));

        if (d > 0)
        {
                return {n, v};
        }
        return {n, -v};
}

template <typename Color, typename RandomEngine>
Color random_non_black_color(RandomEngine& engine)
{
        std::uniform_real_distribution<double> urd(0, 1);

        Color color;
        do
        {
                const double r = urd(engine);
                const double g = urd(engine);
                const double b = urd(engine);
                color = Color(r, g, b);
        } while (color.is_black());

        return color;
}
}
