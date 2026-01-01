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

#include "tables.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

namespace ns::noise
{
namespace
{
constexpr PCG::result_type PCG_INIT_VALUE{12345};

template <typename RandomEngine>
std::vector<unsigned> permutation_table(const unsigned size, RandomEngine& engine)
{
        std::vector<unsigned> res(2ull * size);

        const auto middle = res.begin() + res.size() / 2;

        std::iota(res.begin(), middle, 0);
        std::shuffle(res.begin(), middle, engine);
        std::copy(res.begin(), middle, middle);

        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<numerical::Vector<N, T>> gradient_table(const unsigned size, RandomEngine& engine)
{
        std::vector<numerical::Vector<N, T>> res;
        res.reserve(size);

        for (unsigned i = 0; i < size; ++i)
        {
                static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
                // using double to make identical vectors for different types
                const numerical::Vector<N, double> random = sampling::uniform_on_sphere<N, double>(engine);
                res.push_back(to_vector<T>(random));
        }

        return res;
}
}

template <std::size_t N, typename T>
NoiseTables<N, T> noise_tables(const unsigned size)
{
        PCG pcg(PCG_INIT_VALUE);

        NoiseTables<N, T> res;

        res.permutations = permutation_table(size, pcg);
        ASSERT(res.permutations.size() == 2ull * size);

        res.gradients = gradient_table<N, T>(size, pcg);
        ASSERT(res.gradients.size() == size);

        return res;
}

#define TEMPLATE(N, T) template NoiseTables<N, T> noise_tables(unsigned);

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
