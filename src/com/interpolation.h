/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "math.h"

#include <array>

template <typename T, typename F>
T interpolation(const T& c00, const T& c10, const T& c01, const T& c11, const F& x, const F& y)
{
        T t0 = interpolation(c00, c10, x);
        T t1 = interpolation(c01, c11, x);
        return interpolation(t0, t1, y);
}

template <typename T, typename F>
T interpolation(const T& c000, const T& c100, const T& c010, const T& c110, const T& c001, const T& c101, const T& c011,
                const T& c111, const F& x, const F& y, const F& z)
{
        T t0 = interpolation(c000, c100, x);
        T t1 = interpolation(c010, c110, x);
        T t2 = interpolation(c001, c101, x);
        T t3 = interpolation(c011, c111, x);
        return interpolation(t0, t1, t2, t3, y, z);
}

template <typename T, typename F>
T interpolation(const T& c0000, const T& c1000, const T& c0100, const T& c1100, const T& c0010, const T& c1010, const T& c0110,
                const T& c1110, const T& c0001, const T& c1001, const T& c0101, const T& c1101, const T& c0011, const T& c1011,
                const T& c0111, const T& c1111, const F& x, const F& y, const F& z, const F& w)
{
        T t0 = interpolation(c0000, c1000, x);
        T t1 = interpolation(c0100, c1100, x);
        T t2 = interpolation(c0010, c1010, x);
        T t3 = interpolation(c0110, c1110, x);
        T t4 = interpolation(c0001, c1001, x);
        T t5 = interpolation(c0101, c1101, x);
        T t6 = interpolation(c0011, c1011, x);
        T t7 = interpolation(c0111, c1111, x);
        return interpolation(t0, t1, t2, t3, t4, t5, t6, t7, y, z, w);
}

template <size_t N, typename T, typename F>
T interpolation(const std::array<T, (1 << N)>& data, const std::array<F, N>& p)
{
        static_assert(N > 0);

        if constexpr (N == 1)
        {
                return interpolation(data[0], data[1], p[0]);
        }

        if constexpr (N == 2)
        {
                T t0 = interpolation(data[0], data[1], p[0]);
                T t1 = interpolation(data[2], data[3], p[0]);
                return interpolation(t0, t1, p[1]);
        }

        if constexpr (N == 3)
        {
                T t0 = interpolation(data[0], data[1], p[0]);
                T t1 = interpolation(data[2], data[3], p[0]);
                T t2 = interpolation(data[4], data[5], p[0]);
                T t3 = interpolation(data[6], data[7], p[0]);
                return interpolation(t0, t1, t2, t3, p[1], p[2]);
        }

        if constexpr (N == 4)
        {
                T t0 = interpolation(data[0], data[1], p[0]);
                T t1 = interpolation(data[2], data[3], p[0]);
                T t2 = interpolation(data[4], data[5], p[0]);
                T t3 = interpolation(data[6], data[7], p[0]);
                T t4 = interpolation(data[8], data[9], p[0]);
                T t5 = interpolation(data[10], data[11], p[0]);
                T t6 = interpolation(data[12], data[13], p[0]);
                T t7 = interpolation(data[14], data[15], p[0]);
                return interpolation(t0, t1, t2, t3, t4, t5, t6, t7, p[1], p[2], p[3]);
        }

        if constexpr (N >= 5)
        {
                constexpr size_t next_n = N - 1;
                std::array<T, (1 << next_n)> tmp_data;
                std::array<F, next_n> tmp_p;

                for (size_t i = 0; i < data.size(); i += 2)
                {
                        tmp_data[i >> 1] = interpolation(data[i], data[i + 1], p[0]);
                }

                for (size_t i = 1; i < p.size(); ++i)
                {
                        tmp_p[i - 1] = p[i];
                }

                return interpolation(tmp_data, tmp_p);
        }
}
