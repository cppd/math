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

#include <array>
#include <cstddef>

namespace ns
{
template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] constexpr T interpolation(const T a, const T b, const T t)
{
        return (1 - t) * a + t * b;
}

template <typename T, typename F>
[[nodiscard]] constexpr T interpolation(const T& c00, const T& c01, const T& c10, const T& c11, const F& x, const F& y)
{
        const T t0 = interpolation(c00, c01, x);
        const T t1 = interpolation(c10, c11, x);
        return interpolation(t0, t1, y);
}

template <typename T, typename F>
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
        const T t0 = interpolation(c000, c001, x);
        const T t1 = interpolation(c010, c011, x);
        const T t2 = interpolation(c100, c101, x);
        const T t3 = interpolation(c110, c111, x);
        return interpolation(t0, t1, t2, t3, y, z);
}

template <std::size_t N, typename T, typename F>
[[nodiscard]] constexpr T interpolation(const std::array<T, (1 << N)>& data, const std::array<F, N>& p)
{
        if constexpr (N == 1)
        {
                return interpolation(data[0], data[1], p[0]);
        }
        else if constexpr (N == 2)
        {
                const T t0 = interpolation(data[0], data[1], p[0]);
                const T t1 = interpolation(data[2], data[3], p[0]);
                return interpolation(t0, t1, p[1]);
        }
        else if constexpr (N == 3)
        {
                const T t0 = interpolation(data[0], data[1], p[0]);
                const T t1 = interpolation(data[2], data[3], p[0]);
                const T t2 = interpolation(data[4], data[5], p[0]);
                const T t3 = interpolation(data[6], data[7], p[0]);
                return interpolation(t0, t1, t2, t3, p[1], p[2]);
        }
        else if constexpr (N == 4)
        {
                const T t0 = interpolation(data[0], data[1], p[0]);
                const T t1 = interpolation(data[2], data[3], p[0]);
                const T t2 = interpolation(data[4], data[5], p[0]);
                const T t3 = interpolation(data[6], data[7], p[0]);
                const T t4 = interpolation(data[8], data[9], p[0]);
                const T t5 = interpolation(data[10], data[11], p[0]);
                const T t6 = interpolation(data[12], data[13], p[0]);
                const T t7 = interpolation(data[14], data[15], p[0]);
                return interpolation(t0, t1, t2, t3, t4, t5, t6, t7, p[1], p[2], p[3]);
        }
        else if constexpr (N >= 5)
        {
                constexpr std::size_t NEXT_N = N - 1;
                std::array<T, (1 << NEXT_N)> tmp_data;
                std::array<F, NEXT_N> tmp_p;

                for (std::size_t i = 0; i < data.size(); i += 2)
                {
                        tmp_data[i >> 1] = interpolation(data[i], data[i + 1], p[0]);
                }

                for (std::size_t i = 1; i < p.size(); ++i)
                {
                        tmp_p[i - 1] = p[i];
                }

                return interpolation(tmp_data, tmp_p);
        }
        else
        {
                static_assert(false);
        }
}
}
