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
#include <src/com/interpolation_smooth.h>
#include <src/com/random/pcg.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <numeric>
#include <vector>

namespace ns::numerical
{
namespace
{
constexpr unsigned SIZE = 256;
constexpr Smooth INTERPOLATION_TYPE{Smooth::N_2};
constexpr PCG::result_type PCG_INIT_VALUE{12345};

static_assert(std::has_single_bit(SIZE));
constexpr unsigned SIZE_M = SIZE - 1;

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
        static_assert(N > 0);
        static_assert(std::is_floating_point_v<T>);

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

        [[nodiscard]] T compute(const Vector<N, T>& p) const
        {
                std::array<std::array<T, N>, 2> s;
                std::array<std::array<unsigned, N>, 2> cell;

                for (std::size_t i = 0; i < N; ++i)
                {
                        const T floor = std::floor(p[i]);

                        s[0][i] = p[i] - floor;
                        s[1][i] = s[0][i] - 1;

                        cell[0][i] = static_cast<long long>(floor) & SIZE_M;
                        cell[1][i] = cell[0][i] + 1;
                }

                std::array<T, (1 << N)> data;

                for (std::size_t i = 0; i < data.size(); ++i)
                {
                        Vector<N, T> v;
                        unsigned hash = 0;
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                const bool bit = ((1 << n) & i) != 0;
                                v[n] = s[bit][n];
                                hash = perm_[cell[bit][n] + hash];
                        }
                        data[i] = dot(gradients_[hash], v);
                }

                return interpolation<INTERPOLATION_TYPE>(data, s[0]);
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
