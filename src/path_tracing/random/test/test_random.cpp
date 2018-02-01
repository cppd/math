/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "test_random.h"

#include "com/error.h"
#include "com/log.h"
#include "com/random.h"
#include "com/time.h"
#include "path_tracing/random/complement.h"
#include "path_tracing/random/random_vector.h"

#include <map>

namespace
{
void test_distribution()
{
        constexpr double DISCRETIZATION = 100;
        constexpr int SAMPLE_COUNT = 10'000'000;
        constexpr double DISCREPANCY_LIMIT = 0.01;

        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
        std::map<double, double> data;

        for (int i = 0; i <= DISCRETIZATION; ++i)
        {
                data[i] = 0;
        }

        std::uniform_real_distribution<double> urd(-1, 1);
        vec3 normal = normalize(vec3(urd(random_engine), urd(random_engine), urd(random_engine)));

        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
                vec3 random_vector = normalize(random_hemisphere_cosine_any_length(random_engine, normal));
                double dot_product = dot(random_vector, normal);
                ++data[std::ceil(dot_product * DISCRETIZATION)];
        }

        double max = std::numeric_limits<double>::lowest();
        for (auto i : data)
        {
                max = std::max(max, i.second);
        }
        for (auto& i : data)
        {
                i.second /= max;
        }

        for (auto i = data.crbegin(); i != data.crend(); ++i)
        {
                const auto & [ dot_product, value ] = *i;

                double cosine = dot_product / DISCRETIZATION;

                double angle = std::acos(cosine);
                double degree = angle / PI * 180;

                double discrepancy = std::abs(value - cosine);

                LOG("angle = " + to_string(degree, 5) + ", cos = " + to_string(cosine, 5) + ", val = " + to_string(value, 5) +
                    ", " + to_string(discrepancy, 5));

                if (discrepancy > DISCREPANCY_LIMIT)
                {
                        error("Huge discrepancy");
                }
        }
}

void test_speed()
{
        constexpr int COUNT = 10'000'000;

        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());

        std::uniform_real_distribution<double> urd(-1, 1);

        std::vector<vec3> data;
        for (int i = 0; i < COUNT; ++i)
        {
                data.push_back(normalize(vec3(urd(random_engine), urd(random_engine), urd(random_engine))));
        }

        double start_time = time_in_seconds();

        // Сумма для того, чтобы избежать оптимизаций компилятора из-за неиспользуемого значения функции.
        vec3 sum(0);
        for (const vec3& n : data)
        {
                sum += random_hemisphere_cosine_any_length(random_engine, n);
        }

        LOG("Time = " + to_string_fixed(time_in_seconds() - start_time, 5) + " seconds, sum = " + to_string(sum));
}

template <size_t N, typename T, bool GramSchmidt>
void test_complement(int count)
{
        constexpr T MAX_DOT = 1e-6;
        constexpr T MAX_LENGTH = 1e-6;

        LOG("Test complement in " + to_string(N) + "D: " + (GramSchmidt ? "Gram-Schmidt" : "Subspace"));

        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
        std::uniform_real_distribution<T> urd(-1, 1);

        double start_time = time_in_seconds();

        for (int n = 0; n < count; ++n)
        {
                Vector<N, T> norm = normalize(random_vector<N, T>(random_engine, urd));

                std::array<Vector<N, T>, N - 1> ortho = GramSchmidt ? orthogonal_complement_of_unit_vector_by_gram_schmidt(norm) :
                                                                      orthogonal_complement_of_unit_vector_by_subspace(norm);

                for (const Vector<N, T>& v : ortho)
                {
                        if (std::abs(dot(norm, v)) > MAX_DOT)
                        {
                                error("Not orthogonal to norm");
                        }
                        if (std::abs(1 - dot(v, v)) > MAX_LENGTH)
                        {
                                error("Not unit vector");
                        }
                }

                for (unsigned i = 0; i < ortho.size(); ++i)
                {
                        for (unsigned j = i + 1; j < ortho.size(); ++j)
                        {
                                if (std::abs(dot(ortho[i], ortho[j])) > MAX_DOT)
                                {
                                        error("Not orthogonal to result");
                                }
                        }
                }
        }

        LOG("Time = " + to_string_fixed(time_in_seconds() - start_time, 5) + " seconds");
}

void test_complement()
{
        test_complement<3, double, false>(1000000);
        test_complement<3, double, true>(1000000);
        test_complement<4, double, false>(1000000);
        test_complement<4, double, true>(1000000);
        test_complement<5, double, false>(1000000);
        test_complement<5, double, true>(1000000);
        test_complement<6, double, false>(1000000);
        test_complement<6, double, true>(1000000);
}
}

void test_random()
{
        test_complement();
        LOG("---");
        test_distribution();
        LOG("---");
        test_speed();
}
