/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "noise.h"

#include "tables.h"

#include <src/com/interpolation_smooth.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>

namespace ns::noise
{
namespace
{
constexpr unsigned SIZE = 256;
constexpr Smooth INTERPOLATION_TYPE{Smooth::N_2};

static_assert(std::has_single_bit(SIZE));
constexpr unsigned SIZE_M = SIZE - 1;

template <std::size_t N, typename T>
class Noise final
{
        static_assert(N > 0);
        static_assert(std::is_floating_point_v<T>);

        NoiseTables<N, T> tables_ = noise_tables<N, T>(SIZE);
        T max_reciprocal_ = 2 / std::sqrt(T{N});

public:
        [[nodiscard]] T compute(const numerical::Vector<N, T>& p) const
        {
                std::array<std::array<T, 2>, N> s;
                std::array<std::array<unsigned, 2>, N> cell;
                std::array<T, N> ps;

                for (std::size_t i = 0; i < N; ++i)
                {
                        const T floor = std::floor(p[i]);

                        s[i][0] = p[i] - floor;
                        s[i][1] = s[i][0] - 1;

                        ps[i] = s[i][0];

                        cell[i][0] = static_cast<long long>(floor) & SIZE_M;
                        cell[i][1] = cell[i][0] + 1;
                }

                std::array<T, (1 << N)> data;

                for (std::size_t i = 0; i < data.size(); ++i)
                {
                        numerical::Vector<N, T> v;
                        unsigned hash = 0;
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                const bool bit = ((1 << n) & i) != 0;
                                v[n] = s[n][bit];
                                hash = tables_.permutations[cell[n][bit] + hash];
                        }
                        data[i] = dot(tables_.gradients[hash], v);
                }

                return max_reciprocal_ * interpolation<INTERPOLATION_TYPE>(data, ps);
        }
};
}

template <std::size_t N, typename T>
T noise(const numerical::Vector<N, T>& p)
{
        static const Noise<N, T> noise;

        return noise.compute(p);
}

#define TEMPLATE(N, T) template T noise(const numerical::Vector<N, T>&);

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
