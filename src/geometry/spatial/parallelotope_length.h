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

#pragma once

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace ns::geometry::spatial
{
namespace parallelotope_length_implementation
{
template <std::size_t N>
inline constexpr int DIAGONAL_COUNT = 1 << (N - 1);

template <int INDEX, std::size_t N, std::size_t M, typename T, typename F>
void length(const Vector<N, T>& sum, const std::array<Vector<N, T>, M>& vectors, const F& f)
{
        if constexpr (INDEX >= 0)
        {
                length<INDEX - 1>(sum + vectors[INDEX], vectors, f);
                length<INDEX - 1>(sum - vectors[INDEX], vectors, f);
        }
        else
        {
                f(sum);
        }
}
}

template <std::size_t N, typename T, std::size_t M>
T parallelotope_length(const std::array<Vector<N, T>, M>& vectors)
{
        static_assert(N > 0);
        static_assert(M > 0 && M <= N);

        namespace impl = parallelotope_length_implementation;

        T max_squared = Limits<T>::lowest();

        unsigned count = 0;

        const auto f = [&max_squared, &count](const Vector<N, T>& d)
        {
                ++count;
                max_squared = std::max(max_squared, d.norm_squared());
        };

        // compute all diagonals and find the diagonal with the maximum length
        constexpr int LAST_INDEX = M - 1;
        impl::length<LAST_INDEX - 1>(vectors[LAST_INDEX], vectors, f);

        ASSERT(count == impl::DIAGONAL_COUNT<M>);

        return std::sqrt(max_squared);
}
}
