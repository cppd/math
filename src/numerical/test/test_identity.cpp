/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../identity.h"

#include <cstddef>

namespace ns::numerical
{
template <std::size_t N, typename T>
constexpr bool test()
{
        constexpr auto& A = IDENTITY_ARRAY<N, T>;

        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(A[i][i] == 1))
                {
                        return false;
                }
                for (std::size_t j = 0; j < N; ++j)
                {
                        if (i != j && !(A[i][j] == 0))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N>
constexpr bool test()
{
        static_assert(test<N, int>());
        static_assert(test<N, float>());
        static_assert(test<N, double>());
        return true;
}

static_assert(test<2>());
static_assert(test<3>());
static_assert(test<4>());
static_assert(test<5>());
}
