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

#pragma once

#include <src/com/math.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace ns::color
{
template <typename Derived, std::size_t N, typename T>
class Samples
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<N, T> data_;

protected:
        template <typename... Args>
        explicit constexpr Samples(Args... args)
                : data_(args...)
        {
                static_assert(std::is_base_of_v<Samples, Derived>);
                static_assert(std::is_final_v<Derived>);
                static_assert(sizeof(Samples) == sizeof(Derived));
                static_assert(std::is_trivially_copyable_v<Derived>);
                static_assert(std::is_trivially_destructible_v<Derived>);
        }

        Samples(const Samples&) = default;
        Samples& operator=(const Samples&) = default;
        Samples(Samples&&) = default;
        Samples& operator=(Samples&&) = default;

        ~Samples() = default;

        [[nodiscard]] constexpr const numerical::Vector<N, T>& data() const
        {
                return data_;
        }

        [[nodiscard]] std::string to_string(const std::string_view name) const
        {
                std::ostringstream oss;
                oss.precision(Limits<T>::max_digits10());
                oss << name;
                oss << '(';
                oss << data_[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        oss << ", " << data_[i];
                }
                oss << ')';
                return oss.str();
        }

public:
        void multiply_add(const Derived& a, const T& b)
        {
                data_.multiply_add(a.data_, b);
        }

        void multiply_add(const Derived& a, const Derived& b)
        {
                data_.multiply_add(a.data_, b.data_);
        }

        void multiply_add(const T& b, const Derived& a)
        {
                multiply_add(a, b);
        }

        [[nodiscard]] Derived clamp(const T& low, const T& high) const
        {
                Derived res;
                res.data_ = data_.clamp(low, high);
                return res;
        }

        [[nodiscard]] Derived max_n(const T& v) const
        {
                Derived res;
                res.data_ = data_.max_n(v);
                return res;
        }

        [[nodiscard]] bool is_black() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(data_[0] <= 0))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool has_nan() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (std::isnan(data_[i]))
                        {
                                return true;
                        }
                }
                return false;
        }

        [[nodiscard]] bool is_finite() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!std::isfinite(data_[i]))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool is_non_negative() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(data_[i] >= 0))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool is_in_range(const T& low, const T& high) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(data_[i] >= low && data_[i] <= high))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] constexpr bool equal_to_relative(const Derived& c, const T& relative_error) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T c1 = data_[i];
                        const T c2 = c.data_[i];
                        if (c1 == c2)
                        {
                                continue;
                        }
                        const T abs = absolute(c1 - c2);
                        const T max = std::max(absolute(c1), absolute(c2));
                        if (!(abs / max <= relative_error))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] constexpr bool equal_to_absolute(const Derived& c, const T& absolute_error) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T c1 = data_[i];
                        const T c2 = c.data_[i];
                        if (c1 == c2)
                        {
                                continue;
                        }
                        const T abs = absolute(c1 - c2);
                        if (!(abs <= absolute_error))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool less_than(const Derived& c, const T& relative_error) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T c1 = data_[i];
                        const T c2 = c.data_[i];
                        if (c1 <= c2)
                        {
                                continue;
                        }
                        const T max = std::max(std::abs(c1), std::abs(c2));
                        if (!(std::abs(c1 - c2) / max < relative_error))
                        {
                                return false;
                        }
                }
                return true;
        }

        //

        constexpr Derived& operator+=(const Derived& c) &
        {
                data_ += c.data_;
                return *static_cast<Derived*>(this);
        }

        constexpr Derived& operator-=(const Derived& c) &
        {
                data_ -= c.data_;
                return *static_cast<Derived*>(this);
        }

        constexpr Derived& operator*=(const Derived& c) &
        {
                data_ *= c.data_;
                return *static_cast<Derived*>(this);
        }

        constexpr Derived& operator*=(const T& b) &
        {
                data_ *= b;
                return *static_cast<Derived*>(this);
        }

        constexpr Derived& operator/=(const T& b) &
        {
                data_ /= b;
                return *static_cast<Derived*>(this);
        }

        //

        [[nodiscard]] friend constexpr bool operator==(const Derived& a, const Derived& b)
        {
                return a.data_ == b.data_;
        }

        [[nodiscard]] friend Derived interpolation(const Derived& a, const Derived& b, const T& t)
        {
                Derived res;
                res.data_ = interpolation(a.data_, b.data_, t);
                return res;
        }

        [[nodiscard]] friend Derived max(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = max(a.data_, b.data_);
                return res;
        }

        [[nodiscard]] friend Derived min(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = min(a.data_, b.data_);
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator+(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = a.data_ + b.data_;
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator-(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = a.data_ - b.data_;
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator*(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = a.data_ * b.data_;
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator*(const Derived& a, const T& b)
        {
                Derived res;
                res.data_ = a.data_ * b;
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator*(const T& b, const Derived& a)
        {
                return a * b;
        }

        [[nodiscard]] friend constexpr Derived operator/(const Derived& a, const T& b)
        {
                Derived res;
                res.data_ = a.data_ / b;
                return res;
        }

        [[nodiscard]] friend constexpr Derived operator/(const Derived& a, const Derived& b)
        {
                Derived res;
                res.data_ = a.data_ / b.data_;
                return res;
        }
};
}
