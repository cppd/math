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
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace ns::sampling
{
template <std::size_t N, typename T>
class StratifiedJitteredSampler
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        static int one_dimension_size(int sample_count)
        {
                if (sample_count < 1)
                {
                        error("Stratified jittered sampler: sample count (" + to_string(sample_count)
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

                error("Stratified jittered sampler: failed to compute one dimension sample count for "
                      + to_string(sample_count) + " samples in " + space_name(N));
        }

        static std::vector<T> make_offsets(T min, T max, int sample_count)
        {
                if (!(min < max))
                {
                        error("Stratified jittered sampler: min " + to_string(min) + " must be greater than max "
                              + to_string(max));
                }

                if (sample_count < 1)
                {
                        error("Stratified jittered sampler: one dimension sample count (" + to_string(sample_count)
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
                                error("Stratified jittered sampler: error creating offset values " + to_string(prev)
                                      + " and " + to_string(next));
                        }
                }

                return offsets;
        }

        template <std::size_t M>
        static void product(int count, std::array<int, N>* tuple, std::vector<std::array<int, N>>* result)
        {
                static_assert(N > 0 && M >= 0 && M < N);

                for (int i = 0; i < count; ++i)
                {
                        (*tuple)[M] = i;
                        if constexpr (M == 0)
                        {
                                result->push_back(*tuple);
                        }
                        else
                        {
                                product<M - 1>(count, tuple, result);
                        }
                }
        }

        static std::vector<std::array<int, N>> product(int count)
        {
                ASSERT(count >= 1);

                std::vector<std::array<int, N>> result;
                std::array<int, N> tuple;

                product<N - 1>(count, &tuple, &result);
                ASSERT(result.size() == std::pow(count, N));

                return result;
        }

        std::vector<T> m_offsets;
        std::vector<std::array<int, N>> m_indices;
        T m_min;
        T m_max;
        bool m_shuffle;

public:
        StratifiedJitteredSampler(
                std::type_identity_t<T> min,
                std::type_identity_t<T> max,
                int sample_count,
                bool shuffle)
                : m_offsets(make_offsets(min, max, one_dimension_size(sample_count))),
                  m_indices(product(m_offsets.size() - 1)),
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
                samples->resize(m_indices.size());

                for (std::size_t i = 0; i < m_indices.size(); ++i)
                {
                        const std::array<int, N>& indices = m_indices[i];
                        Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                T min = m_offsets[indices[n]];
                                T max = m_offsets[indices[n] + 1];
                                sample[n] = std::uniform_real_distribution<T>(min, max)(random_engine);
                        }
                }

                if (m_shuffle)
                {
                        std::shuffle(samples->begin(), samples->end(), random_engine);
                }
        }
};
}
