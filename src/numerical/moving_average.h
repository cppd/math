/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace ns::numerical
{
template <typename T>
class MovingAverage final
{
        [[nodiscard]] static auto to_data_type(const std::size_t size)
        {
                if constexpr (requires { std::declval<T>().data(); })
                {
                        using Value = std::remove_pointer_t<decltype(std::declval<T>().data())>;
                        return static_cast<std::remove_cvref_t<Value>>(size);
                }
                else
                {
                        return static_cast<T>(size);
                }
        }

        std::size_t window_size_;
        std::vector<T> data_;
        std::size_t n_ = -1;
        T mean_{0};

public:
        explicit MovingAverage(const std::size_t window_size)
                : window_size_(window_size)
        {
                if (!(window_size > 0))
                {
                        error("Window size must be greater than 0");
                }
                data_.reserve(window_size_);
        }

        void push(const T& value)
        {
                if (data_.size() < window_size_)
                {
                        data_.push_back(value);
                        ++n_;
                        mean_ += (value - mean_) / to_data_type(data_.size());
                        return;
                }

                n_ = (n_ + 1) % window_size_;
                const T old_value = data_[n_];
                data_[n_] = value;

                mean_ += (value - old_value) / to_data_type(window_size_);
        }

        [[nodiscard]] std::size_t size() const
        {
                return data_.size();
        }

        [[nodiscard]] bool has_average() const
        {
                return !data_.empty();
        }

        [[nodiscard]] T average() const
        {
                ASSERT(!data_.empty());
                return mean_;
        }
};
}
