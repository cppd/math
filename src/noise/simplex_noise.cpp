/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/sort.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <bit>
#include <cstddef>

namespace ns::noise
{
namespace
{
constexpr unsigned SIZE = 256;

static_assert(std::has_single_bit(SIZE));
constexpr unsigned SIZE_M = SIZE - 1;

template <std::size_t N, typename T>
T sum(const numerical::Vector<N, T>& p)
{
        T res = p[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                res += p[i];
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> floor(const numerical::Vector<N, T>& p)
{
        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::floor(p[i]);
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> skew(const numerical::Vector<N, T>& p)
{
        static const T f = (std::sqrt(T{N} + 1) - 1) / N;
        return p + numerical::Vector<N, T>(f * sum(p));
}

template <std::size_t N, typename T>
numerical::Vector<N, T> unskew(const numerical::Vector<N, T>& p)
{
        static const T g = (1 - 1 / std::sqrt(T{N} + 1)) / N;
        return p - numerical::Vector<N, T>(g * sum(p));
}

template <typename T>
struct Coordinate final
{
        T value;
        int index;

        bool operator<(const Coordinate& v) const
        {
                ASSERT(value >= 0 && v.value >= 0);
                // sort form the highest magnitude to the lowest magnitude
                return value > v.value;
        }
};

template <std::size_t N, typename T>
std::array<int, N> traversal_indices(const numerical::Vector<N, T>& skewed_cell_coord)
        requires (N == 2)
{
        if (skewed_cell_coord[0] > skewed_cell_coord[1])
        {
                return {0, 1};
        }
        return {1, 0};
}

template <std::size_t N, typename T>
std::array<int, N> traversal_indices(const numerical::Vector<N, T>& skewed_cell_coord)
        requires (N >= 3)
{
        std::array<Coordinate<T>, N> coordinates;
        for (std::size_t i = 0; i < N; ++i)
        {
                coordinates[i] = {.value = skewed_cell_coord[i], .index = static_cast<int>(i)};
        }

        sort(coordinates);

        std::array<int, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = coordinates[i].index;
        }
        return res;
}

template <std::size_t N, typename T>
class SimplexNoise final
{
        static_assert(N > 0);
        static_assert(std::is_floating_point_v<T>);

        NoiseTables<N, T> tables_ = noise_tables<N, T>(SIZE);

        [[nodiscard]] numerical::Vector<N, T> gradient(const numerical::Vector<N, T>& p) const
        {
                unsigned hash = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        hash = tables_.permutations[hash + (static_cast<long long>(p[i]) & SIZE_M)];
                }
                return tables_.gradients[hash];
        }

        void add_contribution(
                const numerical::Vector<N, T>& p,
                const numerical::Vector<N, T>& skewed_corner,
                T* const sum) const
        {
                const numerical::Vector<N, T> coord = p - unskew(skewed_corner);
                const T t = T{0.5} - coord.norm_squared();
                if (t > 0)
                {
                        *sum += power<4>(t) * dot(gradient(skewed_corner), coord);
                }
        }

        [[nodiscard]] T contributions(
                const numerical::Vector<N, T>& p,
                const numerical::Vector<N, T>& skewed_cell_org,
                const numerical::Vector<N, T>& skewed_cell_coord) const
        {
                const std::array<int, N> indices = traversal_indices(skewed_cell_coord);

                T res = 0;
                numerical::Vector<N, T> skewed_corner = skewed_cell_org;

                add_contribution(p, skewed_corner, &res);
                for (std::size_t i = 0; i < N; ++i)
                {
                        ++skewed_corner[indices[i]];
                        add_contribution(p, skewed_corner, &res);
                }
                return res;
        }

public:
        [[nodiscard]] T compute(const numerical::Vector<N, T>& p) const
        {
                const numerical::Vector<N, T> skewed_coord = skew(p);
                const numerical::Vector<N, T> skewed_cell_org = floor(skewed_coord);
                const numerical::Vector<N, T> skewed_cell_coord = skewed_coord - skewed_cell_org;

                return contributions(p, skewed_cell_org, skewed_cell_coord);
        }
};
}

template <std::size_t N, typename T>
T simplex_noise(const numerical::Vector<N, T>& p)
{
        static const SimplexNoise<N, T> noise;

        return noise.compute(p);
}

#define TEMPLATE(N, T) template T simplex_noise(const numerical::Vector<N, T>&);

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
