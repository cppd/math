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

/*
 По книге

 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.

 7.3 Stratified sampling.
*/

#pragma once

#include "com/error.h"
#include "path_tracing/random/random_vector.h"

#include <random>
#include <type_traits>
#include <utility>

// Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
// Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.
// Функция std::shuffle не подходит, так как надо по отдельному измерению.
template <size_t N, typename T, typename RandomEngine>
void shuffle_one_dimension(RandomEngine& random_engine, unsigned dimension, std::vector<Vector<N, T>>* v)
{
        ASSERT(dimension < N);
        ASSERT(v->size() > 0);

        using Distribution = std::uniform_int_distribution<size_t>;

        Distribution distribution;
        for (size_t i = v->size() - 1; i > 0; --i)
        {
                size_t j = distribution(random_engine, Distribution::param_type(0, i));
                std::swap((*v)[i][dimension], (*v)[j][dimension]);
        }
}

template <size_t N, typename T, typename RandomEngine>
void stratified_jittered_samples(RandomEngine& random_engine, int one_dimension_sample_count, std::vector<Vector<N, T>>* samples)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        ASSERT(one_dimension_sample_count > 0);

        T reciprocal_1d_sample_count = static_cast<T>(1) / one_dimension_sample_count;

        std::uniform_real_distribution<T> urd(0, reciprocal_1d_sample_count);

        samples->resize(power<N>(static_cast<unsigned>(one_dimension_sample_count)));

        Vector<N, int> digits(0);
        Vector<N, T> sample(0);
        unsigned sample_num = 0;

        while (true)
        {
                for (int i = 0; i < one_dimension_sample_count; ++i)
                {
                        sample[1] = i * reciprocal_1d_sample_count;

                        for (int j = 0; j < one_dimension_sample_count; ++j)
                        {
                                sample[0] = j * reciprocal_1d_sample_count;

                                (*samples)[sample_num++] = sample + random_vector<N, T>(random_engine, urd);
                        }
                }

                if (N == 2)
                {
                        return;
                }

                for (unsigned i = 2; i < N; ++i)
                {
                        if (digits[i] < one_dimension_sample_count - 1)
                        {
                                ++digits[i];
                                sample[i] = digits[i] * reciprocal_1d_sample_count;
                                break;
                        }
                        if (i == N - 1)
                        {
                                return;
                        }
                        digits[i] = 0;
                        sample[i] = 0;
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
void latin_hypercube_samples(RandomEngine& random_engine, int sample_count, std::vector<Vector<N, T>>* samples)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        ASSERT(sample_count > 0);

        T reciprocal_sample_count = static_cast<T>(1) / sample_count;

        std::uniform_real_distribution<T> urd(0, reciprocal_sample_count);

        samples->resize(sample_count);

        // Случайные числа по диагонали
        for (int i = 0; i < sample_count; ++i)
        {
                (*samples)[i] = Vector<N, T>(i * reciprocal_sample_count) + random_vector<N, T>(random_engine, urd);
        }

        // Достаточно со второго измерения
        for (unsigned i = 1; i < N; ++i)
        {
                shuffle_one_dimension(random_engine, i, samples);
        }
}
