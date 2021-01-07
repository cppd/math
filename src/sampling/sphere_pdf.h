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

#include <src/com/constant.h>

#include <cmath>

namespace ns::sampling
{
template <typename T>
T pdf_sphere_uniform(std::type_identity_t<T> angle)
{
        // ProbabilityDistribution[1, {x, 0, Pi},  Method -> "Normalize"]
        if (angle >= 0 && angle < (PI<T>))
        {
                return 1 / PI<T>;
        }
        return 0;
}

template <typename T>
T pdf_sphere_cosine(std::type_identity_t<T> angle)
{
        // ProbabilityDistribution[Cos[x], {x, 0, Pi/2}, Method -> "Normalize"]
        if (angle >= 0 && angle < (PI<T> / 2))
        {
                return std::cos(angle);
        }
        return 0;
}

template <typename T>
T pdf_sphere_power_cosine(std::type_identity_t<T> angle, std::type_identity_t<T> power)
{
        // Assuming[n >= 0,
        //   ProbabilityDistribution[Cos[x]^n, {x, 0, Pi/2},
        //   Method -> "Normalize"]]
        if (angle >= 0 && angle < (PI<T> / 2))
        {
                T norm = 2 / std::sqrt(PI<T>);
                norm *= std::exp(std::lgamma((2 + power) / 2) - std::lgamma((1 + power) / 2));
                T v = norm * std::pow(std::cos(angle), power);
                return v;
        }
        return 0;
}
}
