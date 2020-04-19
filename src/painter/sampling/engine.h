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

/*
 По книге

 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.

 7.3 Stratified sampling.
*/

#pragma once

#include <src/com/error.h>
#include <src/numerical/random.h>

#include <random>
#include <type_traits>
#include <utility>

namespace painter
{
// Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
// Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.
// Функция std::shuffle не подходит, так как надо по отдельному измерению.
template <size_t N, typename T, typename RandomEngine>
void shuffle_one_dimension(RandomEngine& random_engine, unsigned dimension, std::vector<Vector<N, T>>* v)
{
        ASSERT(dimension < N);
        ASSERT(!v->empty());

        using Distribution = std::uniform_int_distribution<size_t>;

        Distribution distribution;
        for (size_t i = v->size() - 1; i > 0; --i)
        {
                size_t j = distribution(random_engine, Distribution::param_type(0, i));
                std::swap((*v)[i][dimension], (*v)[j][dimension]);
        }
}

template <size_t N, typename T>
class StratifiedJitteredSampleEngine
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        const int m_one_dimension_sample_count;
        const T m_reciprocal_1d_sample_count = static_cast<T>(1) / m_one_dimension_sample_count;

        const int m_sample_count = power<N>(static_cast<unsigned>(m_one_dimension_sample_count));

        std::vector<T> m_offset;

public:
        explicit StratifiedJitteredSampleEngine(int one_dimension_sample_count)
                : m_one_dimension_sample_count(one_dimension_sample_count)
        {
                if (m_one_dimension_sample_count < 1)
                {
                        error("Stratified jittered one dimension sample count ("
                              + to_string(m_one_dimension_sample_count) + ") is not a positive integer");
                }

                m_offset.resize(m_one_dimension_sample_count);

                for (int i = 0; i < m_one_dimension_sample_count; ++i)
                {
                        m_offset[i] = static_cast<T>(i) / m_one_dimension_sample_count;
                }
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                std::uniform_real_distribution<T> urd(0, m_reciprocal_1d_sample_count);

                samples->resize(m_sample_count);

                if constexpr (N == 2)
                {
                        int sample_num = 0;
                        Vector<N, T> sample;

                        for (int i = 0; i < m_one_dimension_sample_count; ++i)
                        {
                                sample[1] = m_offset[i];

                                for (int j = 0; j < m_one_dimension_sample_count; ++j)
                                {
                                        sample[0] = m_offset[j];

                                        (*samples)[sample_num++] = sample + random_vector<N, T>(random_engine, urd);
                                }
                        }

                        ASSERT(sample_num == m_sample_count);

                        return;
                }

                if constexpr (N >= 3)
                {
                        int sample_num = 0;
                        Vector<N, T> sample(0);
                        Vector<N, int> digits(0);

                        while (true)
                        {
                                for (int i = 0; i < m_one_dimension_sample_count; ++i)
                                {
                                        sample[1] = m_offset[i];

                                        for (int j = 0; j < m_one_dimension_sample_count; ++j)
                                        {
                                                sample[0] = m_offset[j];

                                                (*samples)[sample_num++] =
                                                        sample + random_vector<N, T>(random_engine, urd);
                                        }
                                }

                                for (unsigned i = 2; i < N; ++i)
                                {
                                        if (digits[i] < m_one_dimension_sample_count - 1)
                                        {
                                                ++digits[i];
                                                sample[i] = m_offset[digits[i]];
                                                break;
                                        }

                                        if (i == N - 1)
                                        {
                                                ASSERT(sample_num == m_sample_count);

                                                return;
                                        }

                                        digits[i] = 0;
                                        sample[i] = 0;
                                }
                        }
                }
        }
};

template <size_t N, typename T>
class LatinHypercubeSampleEngine
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        const int m_sample_count;
        const T m_reciprocal_sample_count = static_cast<T>(1) / m_sample_count;

public:
        explicit LatinHypercubeSampleEngine(int sample_count) : m_sample_count(sample_count)
        {
                if (m_sample_count < 1)
                {
                        error("Latin hypercube sample count (" + to_string(m_sample_count)
                              + ") is not a positive integer");
                }
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                std::uniform_real_distribution<T> urd(0, m_reciprocal_sample_count);

                samples->resize(m_sample_count);

                // Случайные числа по диагонали
                for (int i = 0; i < m_sample_count; ++i)
                {
                        (*samples)[i] =
                                Vector<N, T>(i * m_reciprocal_sample_count) + random_vector<N, T>(random_engine, urd);
                }

                // Достаточно со второго измерения
                for (unsigned i = 1; i < N; ++i)
                {
                        shuffle_one_dimension(random_engine, i, samples);
                }
        }
};
}
