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

namespace ns::noise
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
        std::vector<unsigned> data(2ull * size);

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
                static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
                // using double to make identical vectors for different types
                const Vector<N, double> random = sampling::uniform_on_sphere<N, double>(engine);
                data.push_back(to_vector<T>(random));
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
        T max_reciprocal_ = 2 / std::sqrt(T{N});

public:
        template <typename RandomEngine>
        explicit Noise(RandomEngine&& engine)
                : perm_(permutation_table(SIZE, engine)),
                  gradients_(gradients<N, T>(SIZE, engine))
        {
                ASSERT(perm_.size() == 2ull * SIZE);
                ASSERT(gradients_.size() == SIZE);
        }

        [[nodiscard]] T compute(const Vector<N, T>& p) const
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
                        Vector<N, T> v;
                        unsigned hash = 0;
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                const bool bit = ((1 << n) & i) != 0;
                                v[n] = s[n][bit];
                                hash = perm_[cell[n][bit] + hash];
                        }
                        data[i] = dot(gradients_[hash], v);
                }

                return max_reciprocal_ * interpolation<INTERPOLATION_TYPE>(data, ps);
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

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
