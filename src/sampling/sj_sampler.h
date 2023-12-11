/*
Copyright (C) 2017-2023 Topological Manifold

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

7.3 Stratified sampling
*/

#pragma once

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns::sampling
{
template <std::size_t N, typename T>
class StratifiedJitteredSampler final
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        static int one_dimension_size(const int sample_count)
        {
                if (sample_count < 1)
                {
                        error("Stratified jittered sampler: sample count (" + to_string(sample_count)
                              + ") is not a positive integer");
                }

                const double v = std::pow(sample_count, 1.0 / N);

                if (const unsigned v_floor = std::floor(v); power<N>(v_floor) >= static_cast<unsigned>(sample_count))
                {
                        return v_floor;
                }

                if (const unsigned v_ceil = std::ceil(v); power<N>(v_ceil) >= static_cast<unsigned>(sample_count))
                {
                        return v_ceil;
                }

                error("Stratified jittered sampler: failed to compute one dimension sample count for "
                      + to_string(sample_count) + " samples in " + space_name(N));
        }

        static std::vector<T> make_offsets(const T min, const T max, const int sample_count)
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
                        const T prev = offsets[i - 1];
                        const T next = offsets[i];
                        if (!(prev < next))
                        {
                                error("Stratified jittered sampler: error creating offset values " + to_string(prev)
                                      + " and " + to_string(next));
                        }
                }

                return offsets;
        }

        template <std::size_t M>
        static void product(
                const int count,
                std::array<int, N>* const tuple,
                std::vector<std::array<int, N>>* const result)
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

        static std::vector<std::array<int, N>> product(const int count)
        {
                ASSERT(count >= 1);

                std::vector<std::array<int, N>> res;
                std::array<int, N> tuple;

                product<N - 1>(count, &tuple, &res);
                ASSERT(res.size() == std::pow(count, N));

                return res;
        }

        std::vector<T> offsets_;
        std::vector<std::array<int, N>> indices_;
        T min_;
        T max_;
        bool shuffle_;

public:
        StratifiedJitteredSampler(
                const std::type_identity_t<T> min,
                const std::type_identity_t<T> max,
                const int sample_count,
                const bool shuffle)
                : offsets_(make_offsets(min, max, one_dimension_size(sample_count))),
                  indices_(product(offsets_.size() - 1)),
                  min_(min),
                  max_(max),
                  shuffle_(shuffle)
        {
        }

        [[nodiscard]] bool shuffled() const
        {
                return shuffle_;
        }

        [[nodiscard]] T min() const
        {
                return min_;
        }

        [[nodiscard]] T max() const
        {
                return max_;
        }

        template <typename RandomEngine>
        void generate(RandomEngine& engine, std::vector<Vector<N, T>>* const samples) const
        {
                constexpr bool IS_FLOAT = std::is_same_v<std::remove_cvref_t<T>, float>;

                samples->resize(indices_.size());

                for (std::size_t i = 0; i < indices_.size(); ++i)
                {
                        const std::array<int, N>& indices = indices_[i];
                        Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                const T min = offsets_[indices[n]];
                                const T max = offsets_[indices[n] + 1];
                                // Distribution may return max if T is float
                                do
                                {
                                        sample[n] = std::uniform_real_distribution<T>(min, max)(engine);
                                } while (IS_FLOAT && sample[n] >= max);
                        }
                }

                if (shuffle_)
                {
                        std::shuffle(samples->begin(), samples->end(), engine);
                }
        }
};
}
