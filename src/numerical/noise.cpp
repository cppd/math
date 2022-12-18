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
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace ns::numerical
{
namespace
{
constexpr unsigned SIZE = 256;
constexpr PCG::result_type PCG_INIT_VALUE{12345};

template <typename RandomEngine>
std::vector<unsigned> permutation_table(const unsigned size, RandomEngine& engine)
{
        std::vector<unsigned> data(2 * size);

        const auto middle = data.begin() + data.size() / 2;

        std::iota(data.begin(), middle, 0);
        std::shuffle(data.begin(), middle, engine);
        std::copy(data.begin(), middle, middle);

        return data;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> gradients(const unsigned size, RandomEngine& engine)
{
        std::vector<Vector<N, T>> data;
        data.reserve(size);

        for (unsigned i = 0; i < size; ++i)
        {
                data.push_back(sampling::uniform_on_sphere<N, T>(engine));
        }

        return data;
}

template <std::size_t N, typename T>
class Noise final
{
        std::vector<unsigned> perm_;
        std::vector<Vector<N, T>> gradients_;

public:
        template <typename RandomEngine>
        explicit Noise(RandomEngine&& engine)
                : perm_(permutation_table(SIZE, engine)),
                  gradients_(gradients<N, T>(SIZE, engine))
        {
                ASSERT(perm_.size() == 2 * SIZE);
                ASSERT(gradients_.size() == SIZE);
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
        static const Noise<N, T> noise{PCG(PCG_INIT_VALUE)};

        return noise.compute(p);
}

#define TEMPLATE(N, T) template T noise(const Vector<N, T>&);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
