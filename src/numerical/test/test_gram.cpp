/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../complement.h"
#include "../gram.h"

#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical
{
namespace
{
template <typename T>
bool are_equal(const T& a, const T& b)
{
        if (a == b)
        {
                return true;
        }
        const T rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return (rel < T(1e-3));
}

template <std::size_t N, typename T, std::size_t Count>
std::array<Vector<N, T>, Count> random_vectors(std::mt19937_64& random_engine)
{
        std::uniform_real_distribution<T> urd(-10, 10);

        std::array<Vector<N, T>, Count> vectors;

        for (Vector<N, T>& v : vectors)
        {
                T norm;
                do
                {
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                v[i] = urd(random_engine);
                        }
                        norm = v.norm_squared();
                        ASSERT(is_finite(norm));
                } while (!(norm > 0));
        }

        return vectors;
}

template <std::size_t N, typename T>
void test_gram_and_complement(std::mt19937_64& random_engine)
{
        static_assert(N >= 2);

        const std::array<Vector<N, T>, N - 1> vectors = random_vectors<N, T, N - 1>(random_engine);

        const T norm_squared = orthogonal_complement(vectors).norm_squared();
        const T gram_determinant = gram_matrix(vectors).determinant();

        if (!are_equal(norm_squared, gram_determinant))
        {
                error("Test <" + to_string(N) + ", " + type_name<T>() + ">, norm squared " + to_string(norm_squared)
                      + " is not equal to Gram determinant " + to_string(gram_determinant));
        }
}

template <std::size_t N, typename T>
void test_gram_and_determinant(std::mt19937_64& random_engine)
{
        static_assert(N >= 1);

        const std::array<Vector<N, T>, N> vectors = random_vectors<N, T, N>(random_engine);

        const T determinant_squared = square(Matrix<N, N, T>(vectors).determinant());
        const T gram_determinant = gram_matrix(vectors).determinant();

        if (!are_equal(determinant_squared, gram_determinant))
        {
                error("Test <" + to_string(N) + ", " + type_name<T>() + ">, determinat squared "
                      + to_string(determinant_squared) + " is not equal to Gram determinant "
                      + to_string(gram_determinant));
        }
}

template <typename T>
void test_gram(std::mt19937_64& random_engine)
{
        test_gram_and_complement<2, T>(random_engine);
        test_gram_and_complement<3, T>(random_engine);
        test_gram_and_complement<4, T>(random_engine);
        test_gram_and_complement<5, T>(random_engine);
        test_gram_and_complement<6, T>(random_engine);
        test_gram_and_complement<7, T>(random_engine);
        test_gram_and_complement<8, T>(random_engine);

        test_gram_and_determinant<1, T>(random_engine);
        test_gram_and_determinant<2, T>(random_engine);
        test_gram_and_determinant<3, T>(random_engine);
        test_gram_and_determinant<4, T>(random_engine);
        test_gram_and_determinant<5, T>(random_engine);
        test_gram_and_determinant<6, T>(random_engine);
        test_gram_and_determinant<7, T>(random_engine);
        test_gram_and_determinant<8, T>(random_engine);
}

void test()
{
        LOG("Test Gram matrix");

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        test_gram<double>(random_engine);
        test_gram<long double>(random_engine);

        LOG("Test Gram matrix passed");
}

TEST_SMALL("Gram matrix", test)
}
}
