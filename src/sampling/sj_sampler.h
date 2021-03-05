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

        template <std::size_t M>
        static void product(
                const std::vector<T>& values,
                std::array<Vector<N, T>, 2>* min_max,
                std::vector<std::array<Vector<N, T>, 2>>* result)
        {
                static_assert(N > 0 && M >= 0 && M < N);

                for (std::size_t i = 0; i + 1 < values.size(); ++i)
                {
                        (*min_max)[0][M] = values[i];
                        (*min_max)[1][M] = values[i + 1];
                        if constexpr (M == 0)
                        {
                                result->push_back(*min_max);
                        }
                        else
                        {
                                product<M - 1>(values, min_max, result);
                        }
                }
        }

        static std::vector<std::array<Vector<N, T>, 2>> product(const std::vector<T>& values)
        {
                ASSERT(values.size() >= 2);

                std::vector<std::array<Vector<N, T>, 2>> result;
                std::array<Vector<N, T>, 2> min_max;

                product<N - 1>(values, &min_max, &result);
                ASSERT(result.size() == std::pow(values.size() - 1, N));

                return result;
        }

        std::vector<std::array<Vector<N, T>, 2>> m_offsets;
        bool m_shuffle;

public:
        StratifiedJitteredSampler(
                std::type_identity_t<T> min,
                std::type_identity_t<T> max,
                int sample_count,
                bool shuffle)
        {
                if (!(min < max))
                {
                        error("Sampler min " + to_string(min) + " must be greater than max " + to_string(max));
                }

                const int one_dimension_sample_count = one_dimension_size(sample_count);
                if (one_dimension_sample_count < 1)
                {
                        error("Stratified jittered sampler: one dimension sample count ("
                              + to_string(one_dimension_sample_count) + ") is not a positive integer");
                }

                std::vector<T> values;
                values.reserve(one_dimension_sample_count + 1);

                const T size = (max - min) / one_dimension_sample_count;
                values.push_back(min);
                for (int i = 1; i < one_dimension_sample_count; ++i)
                {
                        values.push_back(min + i * size);
                }
                values.push_back(max);

                for (std::size_t i = 1; i < values.size(); ++i)
                {
                        if (!(values[i - 1] < values[i]))
                        {
                                error("Error creating values for sampler: " + to_string(values[i - 1]) + ", "
                                      + to_string(values[i]));
                        }
                }

                m_offsets = product(values);
                m_shuffle = shuffle;
        }

        bool shuffled() const
        {
                return m_shuffle;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                samples->resize(m_offsets.size());
                for (std::size_t i = 0; i < m_offsets.size(); ++i)
                {
                        const Vector<N, T>& min = m_offsets[i][0];
                        const Vector<N, T>& max = m_offsets[i][1];
                        Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                sample[n] = std::uniform_real_distribution<T>(min[n], max[n])(random_engine);
                        }
                }
                if (m_shuffle)
                {
                        std::shuffle(samples->begin(), samples->end(), random_engine);
                }
        }
};
}
