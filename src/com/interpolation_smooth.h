/*
Copyright (C) 2017-2022 Topological Manifold

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

namespace ns
{
enum class Smooth
{
        N_1,
        N_2
};

template <Smooth SMOOTH, typename T>
        requires(SMOOTH == Smooth::N_1)
[[nodiscard]] constexpr T smooth(const T t)
{
        return t * t * (3 - 2 * t);
}

template <Smooth SMOOTH, typename T>
        requires(SMOOTH == Smooth::N_2)
[[nodiscard]] constexpr T smooth(const T t)
{
        return t * t * t * (t * (6 * t - 15) + 10);
}

template <Smooth SMOOTH, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const T& c0, const T& c1, const F& t)
{
        return interpolation(c0, c1, smooth<SMOOTH>(t));
}

template <Smooth SMOOTH, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const T& c00, const T& c01, const T& c10, const T& c11, const F& x, const F& y)
{
        return interpolation(c00, c01, c10, c11, smooth<SMOOTH>(x), smooth<SMOOTH>(y));
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
        return interpolation(
                c000, c001, c010, c011, c100, c101, c110, c111, smooth<SMOOTH>(x), smooth<SMOOTH>(y),
                smooth<SMOOTH>(z));
}

template <Smooth SMOOTH, std::size_t N, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const std::array<T, (1 << N)>& data, const std::array<F, N>& p)
{
        std::array<T, N> p_smooth;
        for (std::size_t i = 0; i < N; ++i)
        {
                p_smooth[i] = smooth<SMOOTH>(p[i]);
        }
        return interpolation(data, p_smooth);
}
}
