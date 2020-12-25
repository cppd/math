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
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/numerical/random.h>

#include <random>
#include <type_traits>
#include <utility>

namespace ns::painter
{
template <std::size_t N, typename T>
class StratifiedJitteredSampler
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        std::vector<Vector<N, T>> m_offsets;
        T m_reciprocal_1d_sample_count;

        static int one_dimension_size(int sample_count)
        {
                if (sample_count < 1)
                {
                        error("Stratified jittered sample count (" + to_string(sample_count)
                              + ") is not a positive integer");
                }

                double v = std::pow(sample_count, 1.0 / N);

                if (unsigned v_floor = std::floor(v); power<N>(v_floor) >= static_cast<unsigned>(sample_count))
                {
                        return v_floor;
                }

                if (unsigned v_ceil = std::ceil(v); power<N>(v_ceil) >= static_cast<unsigned>(sample_count))
                {
                        return v_ceil;
                }

                error("Could not compute one dimension sample count for " + to_string(sample_count) + " samples in "
                      + space_name(N));
        }

        template <std::size_t M>
        static void product(const std::vector<T>& values, Vector<N, T>* tuple, std::vector<Vector<N, T>>* result)
        {
                static_assert(N > 0 && M >= 0 && M < N);

                for (T v : values)
                {
                        (*tuple)[M] = v;
                        if constexpr (M == 0)
                        {
                                result->push_back(*tuple);
                        }
                        else
                        {
                                product<M - 1>(values, tuple, result);
                        }
                }
        }

        static std::vector<Vector<N, T>> product(const std::vector<T>& values)
        {
                std::vector<Vector<N, T>> result;
                Vector<N, T> tuple;
                product<N - 1>(values, &tuple, &result);
                ASSERT(result.size() == std::pow(values.size(), N));
                return result;
        }

public:
        explicit StratifiedJitteredSampler(int sample_count)
        {
                int one_dimension_sample_count = one_dimension_size(sample_count);

                if (one_dimension_sample_count < 1)
                {
                        error("Stratified jittered one dimension sample count (" + to_string(one_dimension_sample_count)
                              + ") is not a positive integer");
                }

                std::vector<T> values;
                values.reserve(one_dimension_sample_count);
                for (int i = 0; i < one_dimension_sample_count; ++i)
                {
                        values.push_back(static_cast<T>(i) / one_dimension_sample_count);
                }

                m_offsets = product(values);
                m_reciprocal_1d_sample_count = static_cast<T>(1) / one_dimension_sample_count;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                std::uniform_real_distribution<T> urd(0, m_reciprocal_1d_sample_count);

                samples->resize(m_offsets.size());

                for (std::size_t i = 0; i < m_offsets.size(); ++i)
                {
                        (*samples)[i] = m_offsets[i] + random_vector<N, T>(random_engine, urd);
                }
        }
};

template <std::size_t N, typename T>
class LatinHypercubeSampler
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        const int m_sample_count;
        const T m_reciprocal_sample_count = static_cast<T>(1) / m_sample_count;

        // Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
        // Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.
        // Функция std::shuffle не подходит, так как надо по отдельному измерению.
        template <typename RandomEngine>
        static void shuffle_one_dimension(RandomEngine& random_engine, unsigned dimension, std::vector<Vector<N, T>>* v)
        {
                ASSERT(dimension < N);
                ASSERT(!v->empty());

                using Distribution = std::uniform_int_distribution<std::size_t>;

                Distribution distribution;
                for (std::size_t i = v->size() - 1; i > 0; --i)
                {
                        std::size_t j = distribution(random_engine, Distribution::param_type(0, i));
                        std::swap((*v)[i][dimension], (*v)[j][dimension]);
                }
        }

public:
        explicit LatinHypercubeSampler(int sample_count) : m_sample_count(sample_count)
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
