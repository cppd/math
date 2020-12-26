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
 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.

 7.3 Stratified sampling.
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vec.h>

#include <random>
#include <vector>

namespace ns::random
{
template <std::size_t N, typename T>
class LatinHypercubeSampler
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

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

        std::vector<Vector<N, T>> m_offsets;
        T m_reciprocal_sample_count;

public:
        explicit LatinHypercubeSampler(int sample_count)
        {
                if (sample_count < 1)
                {
                        error("Latin hypercube sampler: sample count (" + to_string(sample_count)
                              + ") is not a positive integer");
                }

                m_offsets.reserve(sample_count);
                for (int i = 0; i < sample_count; ++i)
                {
                        m_offsets.emplace_back(static_cast<T>(i) / sample_count);
                }
                m_reciprocal_sample_count = static_cast<T>(1) / sample_count;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                std::uniform_real_distribution<T> urd(0, m_reciprocal_sample_count);

                samples->resize(m_offsets.size());
                for (std::size_t i = 0; i < m_offsets.size(); ++i)
                {
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                (*samples)[i][n] = m_offsets[i][n] + urd(random_engine);
                        }
                }

                // Достаточно со второго измерения
                for (std::size_t i = 1; i < N; ++i)
                {
                        shuffle_one_dimension(random_engine, i, samples);
                }
        }
};
}
