/*
Copyright (C) 2017-2025 Topological Manifold

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

13.10.1 Multiple importance sampling
*/

#pragma once

#include <src/com/exponent.h>

#include <type_traits>

namespace ns::sampling::mis
{
namespace implementation
{
template <int BETA, typename T>
T power_heuristic(const int f_n, const T f_pdf, const int g_n, const T g_pdf)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(BETA >= 1);

        const T f = power<BETA>(f_n * f_pdf);
        const T g = power<BETA>(g_n * g_pdf);
        return f / (f + g);
}
}

template <typename T>
T balance_heuristic(const int f_n, const T f_pdf, const int g_n, const T g_pdf)
{
        return implementation::power_heuristic<1>(f_n, f_pdf, g_n, g_pdf);
}

template <typename T>
T power_heuristic(const int f_n, const T f_pdf, const int g_n, const T g_pdf)
{
        return implementation::power_heuristic<2>(f_n, f_pdf, g_n, g_pdf);
}
}
