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

#include "error.h"

#include "type/limit.h"

#include <algorithm>
#include <array>

namespace ns
{
// C(n, r) = n! / ((n - r)! * r!)
// std::round(std::exp(std::lgamma(n + 1) - std::lgamma(n - r + 1) - std::lgamma(r + 1)))
template <int N, int R>
inline constexpr int BINOMIAL = []
{
        static_assert(N >= R && R >= 0);

        constexpr unsigned __int128 MAX = limits<int>::max();

        constexpr unsigned __int128 K = (R <= N / 2) ? (N - R) : R;
        unsigned __int128 m = 1;
        for (unsigned __int128 i = N; i > K; --i)
        {
                unsigned __int128 v = m * i;
                if ((v / i) != m)
                {
                        error("Binomial overflow");
                }
                m = v;
        }
        static_assert(N >= K);
        for (unsigned __int128 i = N - K; i > 1; --i)
        {
                m /= i;
        }

        if (m > MAX)
        {
                error("Binomial result overflow");
        }

        return m;
}();

template <int N, int R>
inline constexpr std::array<std::array<unsigned char, R>, BINOMIAL<N, R>> COMBINATIONS = []
{
        static_assert(N >= R && R > 0);

        std::array<unsigned char, N> v;

        std::fill(v.begin(), v.begin() + R, 0);
        std::fill(v.begin() + R, v.end(), 1);

        std::array<std::array<unsigned char, R>, BINOMIAL<N, R>> res;

        for (int row = 0; row < BINOMIAL<N, R>; ++row, std::next_permutation(v.begin(), v.end()))
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
}();
}
