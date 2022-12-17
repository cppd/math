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

#include "noise.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace ns::numerical
{
namespace
{
constexpr unsigned SIZE = 256;

std::vector<unsigned> permutation_table(const unsigned size)
{
        constexpr PCG::result_type PCG_VALUE{12345};

        std::vector<unsigned> data(2 * size);

        const auto middle = data.begin() + data.size() / 2;

        std::iota(data.begin(), middle, 0);
        std::shuffle(data.begin(), middle, PCG(PCG_VALUE));
        std::copy(data.begin(), middle, middle);

        return data;
}

template <std::size_t N, typename T>
class Noise final
{
        std::vector<unsigned> perm_ = permutation_table(SIZE);

public:
        Noise()
        {
                ASSERT(perm_.size() == 2 * SIZE);
        }

        [[nodiscard]] T compute(const Vector<N, T>& /*p*/) const
        {
                error("not implemented");
        }
};
}

template <std::size_t N, typename T>
T noise(const Vector<N, T>& p)
{
        static const Noise<N, T> noise;
        return noise.compute(p);
}

#define TEMPLATE(N, T) template T noise(const Vector<N, T>&);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
