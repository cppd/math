/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "error.h"

#include <algorithm>
#include <array>
#include <tuple>

// Количество сочетаний из n по r: C(n, r) = n! / ((n - r)! * r!).
// Вариант для плавающей точки: std::round(std::exp(std::lgamma(n + 1) - std::lgamma(n - r + 1) - std::lgamma(r + 1))).
constexpr int binomial(int n, int r)
{
        if (n > 29)
        {
                // Если больше 29, то первое умножение не помещается в long long.
                // Для данной программы максимум из 29 достаточен.
                error("Binomial too big");
        }
        if (n < r)
        {
                error("Error binomial");
        }

        long long maximum = (r <= n / 2) ? (n - r) : r;
        long long m = 1;
        for (long long i = n; i > maximum; --i)
        {
                m *= i;
        }
        for (long long i = n - maximum; i > 1; --i)
        {
                m /= i;
        }
        return m;
}

// Список сочетаний по R из последовательности чисел от 0 <= x < N
template <int N, int R>
std::array<std::array<unsigned char, R>, binomial(N, R)> combinations()
{
        std::array<signed char, N> v;

        std::fill(v.begin(), v.begin() + R, 0);
        std::fill(v.begin() + R, v.end(), 1);

        std::array<std::array<unsigned char, R>, binomial(N, R)> res;

        for (int row = 0; row < binomial(N, R); ++row, std::next_permutation(v.begin(), v.end()))
        {
                int cnt = -1;
                for (int i = 0; i < N; ++i)
                {
                        if (v[i] == 0)
                        {
                                res[row][++cnt] = i;
                        }
                }
        }

        return res;
}

template <int N, int R>
std::array<std::tuple<std::array<unsigned char, R>, std::array<unsigned char, N - R>>, binomial(N, R)> combinations_tuple()
{
        std::array<signed char, N> v;

        std::fill(v.begin(), v.begin() + R, 0);
        std::fill(v.begin() + R, v.end(), 1);

        std::array<std::tuple<std::array<unsigned char, R>, std::array<unsigned char, N - R>>, binomial(N, R)> res;

        for (int row = 0; row < binomial(N, R); ++row, std::next_permutation(v.begin(), v.end()))
        {
                int cnt_1 = -1;
                int cnt_2 = -1;
                for (int i = 0; i < N; ++i)
                {
                        if (v[i] == 0)
                        {
                                std::get<0>(res[row])[++cnt_1] = i;
                        }
                        else
                        {
                                std::get<1>(res[row])[++cnt_2] = i;
                        }
                }
        }

        return res;
}
