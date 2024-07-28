/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/print.h>
#include <src/com/random/uniform.h>
#include <src/com/shuffle.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns::sampling
{
template <std::size_t N, typename T>
class LatinHypercubeSampler final
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        static std::vector<T> make_offsets(const T min, const T max, const int sample_count)
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
                        const T prev = offsets[i - 1];
                        const T next = offsets[i];
                        if (!(prev < next))
                        {
                                error("Latin hypercube sampler: error creating offset values " + to_string(prev)
                                      + " and " + to_string(next));
                        }
                }

                return offsets;
        }

        std::vector<T> offsets_;
        std::size_t initial_shuffle_dimension_;
        std::size_t sample_count_;
        T min_;
        T max_;
        bool shuffle_;

public:
        LatinHypercubeSampler(
                const std::type_identity_t<T> min,
                const std::type_identity_t<T> max,
                const int sample_count,
                const bool shuffle)
                : offsets_(make_offsets(min, max, sample_count)),
                  initial_shuffle_dimension_(shuffle ? 0 : 1),
                  sample_count_(sample_count),
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
        void generate(RandomEngine& engine, std::vector<numerical::Vector<N, T>>* const samples) const
        {
                samples->resize(sample_count_);

                for (std::size_t i = 0; i < sample_count_; ++i)
                {
                        const T min = offsets_[i];
                        const T max = offsets_[i + 1];
                        std::uniform_real_distribution<T> urd(min, max);
                        numerical::Vector<N, T>& sample = (*samples)[i];
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                sample[n] = uniform_distribution(engine, urd);
                        }
                }

                for (std::size_t i = initial_shuffle_dimension_; i < N; ++i)
                {
                        shuffle_dimension(engine, i, samples);
                }
        }
};
}
