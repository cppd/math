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

#include "simplex_noise.h"

#include "tables.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/settings/instantiation.h>

#include <bit>

namespace ns::noise
{
namespace
{
constexpr unsigned SIZE = 256;

static_assert(std::has_single_bit(SIZE));
constexpr unsigned SIZE_M = SIZE - 1;

template <std::size_t N, typename T>
T sum(const Vector<N, T>& p)
{
        T res = p[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                res += p[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> floor(const Vector<N, T>& p)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::floor(p[i]);
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> skew(const Vector<N, T>& p)
{
        static const T f = (std::sqrt(T{N} + 1) - 1) / N;
        return p + Vector<N, T>(f * sum(p));
}

template <std::size_t N, typename T>
Vector<N, T> unskew(const Vector<N, T>& p)
{
        static const T g = (1 - 1 / std::sqrt(T{N} + 1)) / N;
        return p - Vector<N, T>(g * sum(p));
}

template <std::size_t N, typename T>
class SimplexNoise final
{
        static_assert(N > 0);
        static_assert(std::is_floating_point_v<T>);

        NoiseTables<N, T> tables_ = noise_tables<N, T>(SIZE);
        T max_reciprocal_ = 2 / std::sqrt(T{N});

        [[nodiscard]] Vector<N, T> gradient(const Vector<N, T>& p) const
        {
                unsigned hash = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        hash = tables_.permutations[hash + (static_cast<long long>(p[i]) & SIZE_M)];
                }
                return tables_.gradients[hash];
        }

        [[nodiscard]] T contibutions(const Vector<N, T>& p, const std::array<Vector<N, T>, N + 1>& skewed_corners) const
        {
                T res = 0;
                for (std::size_t i = 0; i < N + 1; ++i)
                {
                        const Vector<N, T> coord = p - unskew(skewed_corners[i]);
                        const T t = T{0.5} - coord.norm_squared();
                        res += t > 0 ? power<4>(t) * dot(gradient(skewed_corners[i]), coord) : 0;
                }
                return res;
        }

public:
        [[nodiscard]] T compute(const Vector<N, T>& p) const
                requires (N == 2)
        {
                const Vector<N, T> skewed_coord = skew(p);
                const Vector<N, T> skewed_cell_org = floor(skewed_coord);
                const Vector<N, T> skewed_cell_coord = skewed_coord - skewed_cell_org;

                const Vector<2, T> next_corner =
                        (skewed_cell_coord[0] > skewed_cell_coord[1]) ? Vector<2, T>(1, 0) : Vector<2, T>(0, 1);
                const Vector<2, T> last_corner = Vector<2, T>(1, 1);

                const std::array<Vector<N, T>, 3> skewed_corners = {
                        skewed_cell_org, skewed_cell_org + next_corner, skewed_cell_org + last_corner};

                return contibutions(p, skewed_corners);
        }

        [[nodiscard]] T compute(const Vector<N, T>& /*p*/) const
                requires (N >= 3)
        {
                error("not implemented");
        }
};
}

template <std::size_t N, typename T>
T simplex_noise(const Vector<N, T>& p)
{
        static const SimplexNoise<N, T> noise;

        return noise.compute(p);
}

#define TEMPLATE(N, T) template T simplex_noise(const Vector<N, T>&);

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
