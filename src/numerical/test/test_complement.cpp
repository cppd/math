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
#include "../orthogonal.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical
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

template <bool GramSchmidt, std::size_t N, typename T>
std::vector<std::array<Vector<N, T>, N - 1>> complement_vectors(const std::vector<Vector<N, T>>& vectors)
{
        std::vector<std::array<Vector<N, T>, N - 1>> res;
        res.reserve(vectors.size());

        TimePoint start_time = time();

        for (const Vector<N, T>& unit_vector : vectors)
        {
                std::array<Vector<N, T>, N - 1> complement =
                        GramSchmidt ? orthogonal_complement_of_unit_vector_by_gram_schmidt(unit_vector)
                                    : orthogonal_complement_of_unit_vector_by_subspace(unit_vector);

                res.push_back(complement);
        }

        LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " seconds");

        return res;
}

template <std::size_t N, typename T, bool GramSchmidt>
void test_complement(int count)
{
        ASSERT(count > 0);

        LOG("Test complement in " + space_name(N) + ", " + to_string_digit_groups(count) + " " + type_name<T>() + ": "
            + (GramSchmidt ? "Gram-Schmidt" : "Subspace"));

        std::vector<Vector<N, T>> vectors = random_vectors<N, T>(count);

        std::vector<std::array<Vector<N, T>, N - 1>> complements = complement_vectors<GramSchmidt>(vectors);

        ASSERT(vectors.size() == complements.size());

        for (unsigned num = 0; num < vectors.size(); ++num)
        {
                const Vector<N, T>& unit_vector = vectors[num];
                const std::array<Vector<N, T>, N - 1>& complement = complements[num];

                if (!unit_vector.is_unit())
                {
                        error("Not unit vector " + to_string(unit_vector));
                }

                for (const Vector<N, T>& v : complement)
                {
                        if (!is_finite(v))
                        {
                                error("Not finite basis vector " + to_string(v));
                        }

                        if (!vectors_are_orthogonal(unit_vector, v))
                        {
                                error("Orthogonal complement basis is not orthogonal to the input vector ("
                                      + to_string(unit_vector) + ", " + to_string(v) + ")");
                        }

                        if (!v.is_unit())
                        {
                                error("Not orthonormal basis " + to_string(v));
                        }
                }

                for (unsigned i = 0; i < complement.size(); ++i)
                {
                        for (unsigned j = i + 1; j < complement.size(); ++j)
                        {
                                if (!vectors_are_orthogonal(complement[i], complement[j]))
                                {
                                        error("The basis is not orthogonal (" + to_string(complement[i]) + ", "
                                              + to_string(complement[j]) + ")");
                                }
                        }
                }

                Vector<N, T> unit_vector_reconstructed = ortho_nn(complement); // без normalize

                if (!is_finite(unit_vector_reconstructed))
                {
                        error("Not finite reconstructed vector " + to_string(unit_vector_reconstructed));
                }

                if (!unit_vector_reconstructed.is_unit())
                {
                        error("Not unit reconstructed vector " + to_string(unit_vector_reconstructed));
                }

                if (!vectors_are_parallel(unit_vector, unit_vector_reconstructed))
                {
                        error("Orthogonal complement error (" + to_string(unit_vector) + ", "
                              + to_string(unit_vector_reconstructed) + ")");
                }
        }

        LOG("Test passed");
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

TEST_SMALL("Complement", test)
}
}
