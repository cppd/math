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

#include <src/com/radical_inverse.h>
#include <src/numerical/vec.h>

#include <utility>

namespace ns::sampling
{
template <std::size_t N, typename T>
class HaltonSampler final
{
        static constexpr unsigned PRIMES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

        unsigned m_sample = 0;

        template <std::size_t... I>
        static Vector<N, T> generate(unsigned sample, std::integer_sequence<std::size_t, I...>)
        {
                static_assert(N <= std::size(PRIMES));
                static_assert(sizeof...(I) == N);

                return {radical_inverse<PRIMES[I], T>(sample)...};
        }

public:
        HaltonSampler()
        {
        }

        Vector<N, T> generate()
        {
                return generate(m_sample++, std::make_integer_sequence<std::size_t, N>());
        }
};
}
