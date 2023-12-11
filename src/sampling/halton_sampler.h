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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

7.4 The Halton sampler
*/

#pragma once

#include <src/com/radical_inverse.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <utility>

namespace ns::sampling
{
template <std::size_t N, typename T>
class HaltonSampler final
{
        static constexpr std::array PRIMES = std::to_array<unsigned>({2, 3, 5, 7, 11, 13, 17, 19, 23, 29});

        unsigned sample_ = 0;

public:
        HaltonSampler()
        {
        }

        Vector<N, T> generate()
        {
                return []<std::size_t... I>(const unsigned sample, std::integer_sequence<std::size_t, I...>&&)
                {
                        static_assert(sizeof...(I) == N);
                        static_assert(N <= std::size(PRIMES));

                        return Vector<N, T>{radical_inverse<PRIMES[I], T>(sample)...};
                }(sample_++, std::make_integer_sequence<std::size_t, N>());
        }
};
}
