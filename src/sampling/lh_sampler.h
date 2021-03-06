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

namespace ns::sampling
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

        static std::vector<T> make_offsets(T min, T max, int sample_count)
        {
                std::vector<T> offsets;
                offsets.reserve(sample_count + 1);

                const T offset_size = (max - min) / sample_count;
                offsets.push_back(min);
                for (int i = 1; i < sample_count; ++i)
                {
                        offsets.push_back(min + i * offset_size);
                }
                offsets.push_back(max);

                ASSERT(offsets.size() == 1 + static_cast<std::size_t>(sample_count));

                for (std::size_t i = 1; i < offsets.size(); ++i)
                {
                        T prev = offsets[i - 1];
                        T next = offsets[i];
                        if (!(prev < next))
                        {
                                error("Latin hypercube sampler: error creating offset values " + to_string(prev)
                                      + " and " + to_string(next));
                        }
                }

                return offsets;
        }

        bool m_shuffle;
        std::size_t m_initial_shuffle_dimension;
        std::size_t m_sample_count;

        std::vector<T> m_offsets;

public:
        LatinHypercubeSampler(std::type_identity_t<T> min, std::type_identity_t<T> max, int sample_count, bool shuffle)
        {
                if (!(min < max))
                {
                        error("Latin hypercube sampler: min " + to_string(min) + " must be greater than max "
                              + to_string(max));
                }

                if (sample_count < 1)
                {
                        error("Latin hypercube sampler: sample count (" + to_string(sample_count)
                              + ") is not a positive integer");
                }

                m_shuffle = shuffle;
                m_initial_shuffle_dimension = shuffle ? 0 : 1;
                m_sample_count = sample_count;
                m_offsets = make_offsets(min, max, sample_count);
        }

        bool shuffled() const
        {
                return m_shuffle;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                samples->resize(m_sample_count);

                for (std::size_t i = 0; i < m_sample_count; ++i)
                {
                        std::uniform_real_distribution<T> urd(m_offsets[i], m_offsets[i + 1]);
                        Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                sample[n] = urd(random_engine);
                        }
                }

                for (std::size_t i = m_initial_shuffle_dimension; i < N; ++i)
                {
                        shuffle_one_dimension(random_engine, i, samples);
                }
        }
};
}
