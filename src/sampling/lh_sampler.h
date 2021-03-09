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
#include <src/com/shuffle.h>
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

        static std::vector<T> make_offsets(T min, T max, int sample_count)
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

        std::vector<T> m_offsets;
        std::size_t m_initial_shuffle_dimension;
        std::size_t m_sample_count;
        T m_min;
        T m_max;
        bool m_shuffle;

public:
        LatinHypercubeSampler(std::type_identity_t<T> min, std::type_identity_t<T> max, int sample_count, bool shuffle)
                : m_offsets(make_offsets(min, max, sample_count)),
                  m_initial_shuffle_dimension(shuffle ? 0 : 1),
                  m_sample_count(sample_count),
                  m_min(min),
                  m_max(max),
                  m_shuffle(shuffle)
        {
        }

        bool shuffled() const
        {
                return m_shuffle;
        }

        T min() const
        {
                return m_min;
        }

        T max() const
        {
                return m_max;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                constexpr bool IS_FLOAT = std::is_same_v<std::remove_cvref_t<T>, float>;

                samples->resize(m_sample_count);

                for (std::size_t i = 0; i < m_sample_count; ++i)
                {
                        T min = m_offsets[i];
                        T max = m_offsets[i + 1];
                        std::uniform_real_distribution<T> urd(min, max);
                        Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                // Distribution may return max if T is float
                                do
                                {
                                        sample[n] = urd(random_engine);
                                } while (IS_FLOAT && sample[n] >= max);
                        }
                }

                for (std::size_t i = m_initial_shuffle_dimension; i < N; ++i)
                {
                        shuffle_dimension(random_engine, i, samples);
                }
        }
};
}
