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

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/complement.h>
#include <src/numerical/determinant.h>
#include <src/numerical/gram.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>

namespace ns::numerical
{
namespace
{
template <typename T>
[[nodiscard]] bool equal(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return true;
        }
        const T rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <std::size_t N, typename T, std::size_t COUNT, typename RandomEngine>
std::array<Vector<N, T>, COUNT> random_vectors(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);

        std::array<Vector<N, T>, COUNT> res;

        for (Vector<N, T>& v : res)
        {
                T norm;
                do
                {
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                v[i] = urd(engine);
                        }
                        norm = v.norm_squared();
                        ASSERT(std::isfinite(norm));
                } while (!(norm > 0));
        }

        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
void test_gram_and_complement(RandomEngine& engine)
{
        static_assert(N >= 2);

        const std::array<Vector<N, T>, N - 1> vectors = random_vectors<N, T, N - 1>(engine);

        const T norm_squared = orthogonal_complement(vectors).norm_squared();
        const T gram_determinant = gram_matrix(vectors).determinant();

        if (!equal(norm_squared, gram_determinant, T{1e-8}))
        {
                error("Test <" + to_string(N) + ", " + type_name<T>() + ">, norm squared " + to_string(norm_squared)
                      + " is not equal to Gram determinant " + to_string(gram_determinant));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_gram_and_determinant(RandomEngine& engine)
{
        static_assert(N >= 1);

        const std::array<Vector<N, T>, N> vectors = random_vectors<N, T, N>(engine);

        const T determinant_squared = square(determinant(vectors));
        const T gram_determinant = gram_matrix(vectors).determinant();

        if (!equal(determinant_squared, gram_determinant, T{1e-3}))
        {
                error("Test <" + to_string(N) + ", " + type_name<T>() + ">, determinat squared "
                      + to_string(determinant_squared) + " is not equal to Gram determinant "
                      + to_string(gram_determinant));
        }
}

template <typename T, typename RandomEngine>
void test_gram(RandomEngine& engine)
{
        test_gram_and_complement<2, T>(engine);
        test_gram_and_complement<3, T>(engine);
        test_gram_and_complement<4, T>(engine);
        test_gram_and_complement<5, T>(engine);
        test_gram_and_complement<6, T>(engine);
        test_gram_and_complement<7, T>(engine);
        test_gram_and_complement<8, T>(engine);

        test_gram_and_determinant<1, T>(engine);
        test_gram_and_determinant<2, T>(engine);
        test_gram_and_determinant<3, T>(engine);
        test_gram_and_determinant<4, T>(engine);
        test_gram_and_determinant<5, T>(engine);
        test_gram_and_determinant<6, T>(engine);
        test_gram_and_determinant<7, T>(engine);
        test_gram_and_determinant<8, T>(engine);
}

void test()
{
        LOG("Test Gram matrix");

        PCG engine;

        test_gram<double>(engine);
        test_gram<long double>(engine);

        LOG("Test Gram matrix passed");
}

TEST_SMALL("Gram Matrix", test)
}
}
