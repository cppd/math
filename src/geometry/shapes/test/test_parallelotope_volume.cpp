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

#include "../parallelotope_volume.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/complement.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <utility>

namespace ns::geometry::shapes::test
{
namespace
{
template <typename T>
bool equal(const T& a, const T& b, const T& precision)
{
        const T relative = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return relative <= precision;
}

template <std::size_t M, std::size_t N, typename T>
void test(const std::array<Vector<N, T>, N>& vectors, const T& volume, const T& precision)
{
        static_assert(M <= N);

        std::array<Vector<N, T>, M> v;
        for (std::size_t i = 0; i < M; ++i)
        {
                v[i] = vectors[i];
        }

        const T computed_volume = parallelotope_volume(v);
        if (!equal(computed_volume, volume, precision))
        {
                error("Error " + to_string(M) + "-palallelotope volume in " + space_name(N)
                      + ", computed = " + to_string(computed_volume) + ", expected = " + to_string(volume));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test(const T& precision, RandomEngine& engine)
{
        const T scale = std::uniform_real_distribution<T>(0.1, 10)(engine);
        const Vector<N, T> vector = sampling::uniform_on_sphere<N, T>(engine);
        const std::array<Vector<N, T>, N - 1> complement = numerical::orthogonal_complement_of_unit_vector(vector);

        const std::array<Vector<N, T>, N> vectors = [&]
        {
                std::array<Vector<N, T>, N> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = complement[i] * scale;
                }
                res[N - 1] = vector * scale;
                return res;
        }();

        [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                (test<I + 1>(vectors, power<I + 1>(scale), precision), ...);
        }(std::make_integer_sequence<std::size_t, N>());
}

template <typename T, typename RandomEngine>
void test(const T& precision, RandomEngine& engine)
{
        test<3, T>(precision, engine);
        test<4, T>(precision, engine);
        test<5, T>(precision, engine);
        test<6, T>(precision, engine);
}

void test_parallelotope_volume()
{
        LOG("Test parallelotope volume");
        PCG engine;
        test<float>(1e-6, engine);
        test<double>(1e-14, engine);
        test<long double>(1e-18, engine);
        LOG("Test parallelotope volume passed");
}

TEST_SMALL("Parallelotope Volume", test_parallelotope_volume)
}
}
