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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical::complement_implementation
{
namespace
{
constexpr int VECTOR_COUNT = 100'000;

template <typename T>
constexpr T COS_LIMIT_ORTHOGONAL = limits<T>::epsilon() * 100;

template <typename T>
constexpr T COS_LIMIT_PARALLEL = 1 - limits<T>::epsilon() * 100;

template <std::size_t N, typename T>
constexpr bool vectors_are_orthogonal(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return std::abs(dot(a, b)) <= COS_LIMIT_ORTHOGONAL<T>;
}

template <std::size_t N, typename T>
constexpr bool vectors_are_parallel(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return std::abs(dot(a, b)) >= COS_LIMIT_PARALLEL<T>;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_vectors(int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(sampling::uniform_on_sphere<N, T>(random_engine));
        }
        return res;
}

template <bool GRAM_SCHMIDT, std::size_t N, typename T>
std::vector<std::array<Vector<N, T>, N - 1>> complement_vectors(const std::vector<Vector<N, T>>& vectors)
{
        std::vector<std::array<Vector<N, T>, N - 1>> res;
        res.reserve(vectors.size());

        TimePoint start_time = time();

        for (const Vector<N, T>& unit_vector : vectors)
        {
                if (GRAM_SCHMIDT)
                {
                        res.push_back(orthogonal_complement_by_gram_schmidt(unit_vector));
                }
                else
                {
                        res.push_back(orthogonal_complement_by_subspace(unit_vector));
                }
        }

        LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " seconds");

        return res;
}

template <std::size_t N, typename T>
void test_complement(const Vector<N, T>& unit_vector, const std::array<Vector<N, T>, N - 1>& complement)
{
        if (!unit_vector.is_unit())
        {
                error("Not unit input vector " + to_string(unit_vector));
        }

        for (const Vector<N, T>& v : complement)
        {
                if (!is_finite(v))
                {
                        error("Not finite complement vector " + to_string(v));
                }

                if (!v.is_unit())
                {
                        error("Not unit complement vector " + to_string(v));
                }

                if (!vectors_are_orthogonal(unit_vector, v))
                {
                        error("Complement vector " + to_string(v) + " is not orthogonal to the input vector "
                              + to_string(unit_vector));
                }
        }

        for (std::size_t i = 0; i < complement.size(); ++i)
        {
                for (std::size_t j = i + 1; j < complement.size(); ++j)
                {
                        if (!vectors_are_orthogonal(complement[i], complement[j]))
                        {
                                error("Complement vectors are not orthogonal (" + to_string(complement[i]) + ", "
                                      + to_string(complement[j]) + ")");
                        }
                }
        }

        const Vector<N, T> reconstructed = orthogonal_complement(complement);

        if (!is_finite(reconstructed))
        {
                error("Not finite reconstructed vector " + to_string(reconstructed));
        }

        if (!reconstructed.is_unit())
        {
                error("Not unit reconstructed vector " + to_string(reconstructed));
        }

        if (!vectors_are_parallel(unit_vector, reconstructed))
        {
                error("Reconstructed vector " + to_string(reconstructed) + " is not parallel to input vector "
                      + to_string(unit_vector));
        }
}

template <std::size_t N, typename T, bool GRAM_SCHMIDT>
void test_complement(int count)
{
        ASSERT(count > 0);

        LOG("Test complement in " + space_name(N) + ", " + to_string_digit_groups(count) + " " + type_name<T>() + ": "
            + (GRAM_SCHMIDT ? "Gram-Schmidt" : "Subspace"));

        const std::vector<Vector<N, T>> vectors = random_vectors<N, T>(count);

        const std::vector<std::array<Vector<N, T>, N - 1>> complements = complement_vectors<GRAM_SCHMIDT>(vectors);

        ASSERT(vectors.size() == complements.size());

        for (std::size_t i = 0; i < vectors.size(); ++i)
        {
                test_complement(vectors[i], complements[i]);
        }

        LOG("Test complement passed");
}

template <std::size_t N, typename T>
void test_complement(int vector_count)
{
        test_complement<N, T, false>(vector_count);
        test_complement<N, T, true>(vector_count);
}

template <typename T>
void test_complement(int vector_count)
{
        test_complement<2, T>(vector_count);
        LOG("---");
        test_complement<3, T>(vector_count);
        LOG("---");
        test_complement<4, T>(vector_count);
        LOG("---");
        test_complement<5, T>(vector_count);
        LOG("---");
        test_complement<6, T>(vector_count);
}

void test(ProgressRatio* progress)
{
        progress->set(0);
        test_complement<float>(VECTOR_COUNT);
        progress->set(1, 3);
        LOG("---");
        test_complement<double>(VECTOR_COUNT);
        progress->set(2, 3);
        LOG("---");
        test_complement<long double>(VECTOR_COUNT / 100);
        progress->set(3, 3);
}

//

void random_number(mpz_class* v, std::mt19937_64& random_engine)
{
        std::array<unsigned char, 50> data;
        std::uniform_int_distribution<int> uid(0, limits<unsigned char>::max());
        for (unsigned char& c : data)
        {
                c = uid(random_engine);
        }
        mpz_import(v->get_mpz_t(), 1, -1, data.size(), 0, 0, data.data());
        if (std::bernoulli_distribution(0.5)(random_engine))
        {
                mpz_neg(v->get_mpz_t(), v->get_mpz_t());
        }
}

void random_number(long long* v, std::mt19937_64& random_engine)
{
        *v = std::uniform_int_distribution<long long>(-100, 100)(random_engine);
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N - 1> random_vectors(std::mt19937_64& random_engine)
{
        std::array<Vector<N, T>, N - 1> vectors;
        for (Vector<N, T>& v : vectors)
        {
                bool not_zero = false;
                do
                {
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                random_number(&v[i], random_engine);
                                not_zero = (v[i] != 0);
                        }
                } while (!not_zero);
        }
        return vectors;
}

template <std::size_t N, typename T>
T dot_product(const Vector<N, T>& v1, const Vector<N, T>& v2)
{
        T sum = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
                sum += v1[i] * v2[i];
        }
        return sum;
}

template <typename T>
std::string type_name() requires std::is_same_v<mpz_class, T>
{
        return "mpz";
}

template <typename T>
std::string type_name() requires std::is_same_v<long long, T>
{
        return "long long";
}

template <std::size_t N, typename T>
void test_integer_impl()
{
        static_assert(N >= 2);

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        std::array<Vector<N, T>, N - 1> vectors;
        static Vector<N, T> complement;
        int i = 0;
        do
        {
                if (++i > 10)
                {
                        error("Non-zero complement not found, " + type_name<T>());
                }
                vectors = random_vectors<N, T>(random_engine);
                complement = orthogonal_complement(vectors);
        } while (dot_product(complement, complement) == 0);

        for (const Vector<N, T>& v : vectors)
        {
                if (dot_product(complement, v) != 0)
                {
                        error("Complement is not orthogonal, " + type_name<T>());
                }
        }
}

template <typename T>
void test_integer_impl()
{
        test_integer_impl<2, T>();
        test_integer_impl<3, T>();
        test_integer_impl<4, T>();
        test_integer_impl<5, T>();
        test_integer_impl<6, T>();
        test_integer_impl<7, T>();
        test_integer_impl<8, T>();
}

void test_integer()
{
        LOG("Test integer complement");
        test_integer_impl<mpz_class>();
        test_integer_impl<long long>();
        LOG("Test integer complement passed");
}

TEST_SMALL("Complement", test)
TEST_SMALL("Complement, integer", test_integer)
}
}
