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

#pragma once

#include "interpolation.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace ns
{
enum class Smooth
{
        N_0,
        N_1,
        N_2,
        N_3,
        N_4
};

std::string_view smooth_to_string(Smooth smooth);

namespace interpolation_smooth_implementation
{
// Plot[{x, -2*x^3 + 3*x^2,
//   6*x^5 - 15*x^4 + 10*x^3, -20*x^7 + 70*x^6 - 84*x^5 + 35*x^4,
//   70*x^9 - 315*x^8 + 540*x^7 - 420*x^6 + 126*x^5}, {x, 0, 1},
//  PlotLegends -> "Expressions"]

template <Smooth SMOOTH, typename T>
        requires (SMOOTH == Smooth::N_0)
[[nodiscard]] constexpr T smooth(const T t)
{
        static_assert(std::is_floating_point_v<T>);

        return t;
}

template <Smooth SMOOTH, typename T>
        requires (SMOOTH == Smooth::N_1)
[[nodiscard]] constexpr T smooth(const T t)
{
        static_assert(std::is_floating_point_v<T>);

        return t * t * (-2 * t + 3);
}

template <Smooth SMOOTH, typename T>
        requires (SMOOTH == Smooth::N_2)
[[nodiscard]] constexpr T smooth(const T t)
{
        static_assert(std::is_floating_point_v<T>);

        return t * t * t * (t * (6 * t - 15) + 10);
}

template <Smooth SMOOTH, typename T>
        requires (SMOOTH == Smooth::N_3)
[[nodiscard]] constexpr T smooth(const T t)
{
        static_assert(std::is_floating_point_v<T>);

        const T t_2 = t * t;
        return t_2 * t_2 * (t * (t * (-20 * t + 70) - 84) + 35);
}

template <Smooth SMOOTH, typename T>
        requires (SMOOTH == Smooth::N_4)
[[nodiscard]] constexpr T smooth(const T t)
{
        static_assert(std::is_floating_point_v<T>);

        const T t_2 = t * t;
        return t_2 * t_2 * t * (t * (t * (t * (70 * t - 315) + 540) - 420) + 126);
}
}

template <Smooth SMOOTH, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const T& c0, const T& c1, const F& t)
{
        namespace impl = interpolation_smooth_implementation;

        return interpolation(c0, c1, impl::smooth<SMOOTH>(t));
}

template <Smooth SMOOTH, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const T& c00, const T& c01, const T& c10, const T& c11, const F& x, const F& y)
{
        namespace impl = interpolation_smooth_implementation;

        return interpolation(c00, c01, c10, c11, impl::smooth<SMOOTH>(x), impl::smooth<SMOOTH>(y));
}

template <Smooth SMOOTH, typename T, typename F>
[[nodiscard]] constexpr T interpolation(
        const T& c000,
        const T& c001,
        const T& c010,
        const T& c011,
        const T& c100,
        const T& c101,
        const T& c110,
        const T& c111,
        const F& x,
        const F& y,
        const F& z)
{
        namespace impl = interpolation_smooth_implementation;

        return interpolation(
                c000, c001, c010, c011, c100, c101, c110, c111, impl::smooth<SMOOTH>(x), impl::smooth<SMOOTH>(y),
                impl::smooth<SMOOTH>(z));
}

template <Smooth SMOOTH, std::size_t N, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const std::array<T, (1 << N)>& data, const std::array<F, N>& p)
{
        namespace impl = interpolation_smooth_implementation;

        std::array<T, N> p_smooth;
        for (std::size_t i = 0; i < N; ++i)
        {
                p_smooth[i] = impl::smooth<SMOOTH>(p[i]);
        }
        return interpolation(data, p_smooth);
}
}
