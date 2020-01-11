/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "test_complement.h"

#include "com/log.h"
#include "com/math.h"
#include "com/names.h"
#include "com/random/engine.h"
#include "com/random/vector.h"
#include "com/time.h"
#include "com/type/limit.h"
#include "com/type/name.h"
#include "geometry/core/complement.h"
#include "geometry/core/linear_algebra.h"

#include <random>

namespace
{
constexpr int VECTOR_COUNT = 1'000'000;

template <typename T>
constexpr T COS_LIMIT_ORTHOGONAL = limits<T>::epsilon() * 100;

template <typename T>
constexpr T COS_LIMIT_PARALLEL = 1 - limits<T>::epsilon() * 100;

template <typename T>
constexpr T MAX_LENGTH_DISCREPANCY = limits<T>::epsilon() * 100;

template <size_t N, typename T>
constexpr bool vectors_are_orthogonal(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return std::abs(dot(a, b)) <= COS_LIMIT_ORTHOGONAL<T>;
}

template <size_t N, typename T>
constexpr bool vectors_are_parallel(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return std::abs(dot(a, b)) >= COS_LIMIT_PARALLEL<T>;
}

template <size_t N, typename T>
bool vector_is_unit(const Vector<N, T>& v)
{
        return std::abs(1 - v.norm()) <= MAX_LENGTH_DISCREPANCY<T>;
}

template <size_t N, typename T>
std::vector<Vector<N, T>> random_vectors(int count)
{
        RandomEngineWithSeed<std::mt19937_64> random_engine;
        std::uniform_real_distribution<T> urd(-1, 1);

        std::vector<Vector<N, T>> res;
        res.reserve(count);

        for (int i = 0; i < count; ++i)
        {
                while (true)
                {
                        Vector<N, T> v = random_vector<N, T>(random_engine, urd).normalized();
                        if (is_finite(v))
                        {
                                res.push_back(v);
                                break;
                        }
                }
        }

        return res;
}

template <bool GramSchmidt, size_t N, typename T>
std::vector<std::array<Vector<N, T>, N - 1>> complement_vectors(const std::vector<Vector<N, T>>& vectors)
{
        std::vector<std::array<Vector<N, T>, N - 1>> res;
        res.reserve(vectors.size());

        double start_time = time_in_seconds();

        for (const Vector<N, T>& unit_vector : vectors)
        {
                std::array<Vector<N, T>, N - 1> complement =
                        GramSchmidt ? orthogonal_complement_of_unit_vector_by_gram_schmidt(unit_vector) :
                                      orthogonal_complement_of_unit_vector_by_subspace(unit_vector);

                res.push_back(complement);
        }

        LOG("Time = " + to_string_fixed(time_in_seconds() - start_time, 5) + " seconds");

        return res;
}

template <size_t N, typename T, bool GramSchmidt>
void test_complement(int count)
{
        ASSERT(count > 0);

        LOG("Test complement in " + space_name(N) + ", " + to_string_digit_groups(count) + " " + type_name<T>() + ": " +
            (GramSchmidt ? "Gram-Schmidt" : "Subspace"));

        std::vector<Vector<N, T>> vectors = random_vectors<N, T>(count);

        std::vector<std::array<Vector<N, T>, N - 1>> complements = complement_vectors<GramSchmidt>(vectors);

        ASSERT(vectors.size() == complements.size());

        for (unsigned num = 0; num < vectors.size(); ++num)
        {
                const Vector<N, T>& unit_vector = vectors[num];
                const std::array<Vector<N, T>, N - 1>& complement = complements[num];

                ASSERT(vector_is_unit(unit_vector));

                for (const Vector<N, T>& v : complement)
                {
                        if (!is_finite(v))
                        {
                                error("Not finite basis vector");
                        }

                        if (!vectors_are_orthogonal(unit_vector, v))
                        {
                                error("Orthogonal complement basis is not orthogonal to the input vector");
                        }

                        if (!vector_is_unit(v))
                        {
                                error("Not orthonormal basis");
                        }
                }

                for (unsigned i = 0; i < complement.size(); ++i)
                {
                        for (unsigned j = i + 1; j < complement.size(); ++j)
                        {
                                if (!vectors_are_orthogonal(complement[i], complement[j]))
                                {
                                        error("The basis is not orthogonal");
                                }
                        }
                }

                Vector<N, T> unit_vector_reconstructed = ortho_nn(complement); // без normalize

                if (!is_finite(unit_vector_reconstructed))
                {
                        error("Not finite reconstructed vector");
                }

                if (!vector_is_unit(unit_vector_reconstructed))
                {
                        error("Not unit reconstructed vector");
                }

                if (!vectors_are_parallel(unit_vector, unit_vector_reconstructed))
                {
                        error("Orthogonal complement error");
                }
        }

        LOG("Test passed");
}

template <size_t N, typename T>
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
}

void test_complement()
{
        test_complement<float>(VECTOR_COUNT);
        LOG("---");
        test_complement<double>(VECTOR_COUNT);
        LOG("---");
        test_complement<long double>(VECTOR_COUNT / 100);
}
