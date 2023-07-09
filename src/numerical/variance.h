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

#pragma once

#include <src/com/error.h>

#include <vector>

namespace ns::numerical
{
template <typename T>
class MovingVariance final
{
        std::size_t window_size_;
        std::vector<T> data_;
        std::size_t n_ = -1;
        T mean_ = 0;
        T sum_ = 0;

public:
        explicit MovingVariance(const std::size_t window_size)
                : window_size_(window_size)
        {
                if (!(window_size > 0))
                {
                        error("Window size must be greater than 0");
                }
                data_.reserve(window_size_);
        }

        void push(const T value)
        {
                // Based on Welford's algorithm

                if (data_.size() < window_size_)
                {
                        data_.push_back(value);
                        ++n_;
                        const T delta = value - mean_;
                        mean_ += delta / data_.size();
                        sum_ += delta * (value - mean_);
                        return;
                }

                n_ = (n_ + 1) % window_size_;
                const T old_value = data_[n_];
                data_[n_] = value;

                const T old_mean = mean_;
                const T delta = value - old_value;
                mean_ += delta / window_size_;
                sum_ += delta * (value + old_value - mean_ - old_mean);
        }

        [[nodiscard]] bool empty() const
        {
                return data_.empty();
        }

        [[nodiscard]] T mean() const
        {
                ASSERT(!data_.empty());
                return mean_;
        }

        [[nodiscard]] T variance_n() const
        {
                ASSERT(!data_.empty());
                return sum_ / data_.size();
        }

        [[nodiscard]] T variance() const
        {
                ASSERT(data_.size() >= 2);
                return sum_ / (data_.size() - 1);
        }
};
}
