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

#include "vector.h"

#include <src/com/type/limit.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>

namespace ns::numerical
{
template <typename T, bool JPL>
class QuaternionHJ final
{
        static_assert(std::is_floating_point_v<T>);

        Vector<4, T> data_;

        explicit constexpr QuaternionHJ(const Vector<4, T>& v)
                : data_(v)
        {
        }

public:
        constexpr QuaternionHJ()
        {
        }

        constexpr QuaternionHJ(const T w, const Vector<3, T>& v)
                : data_(w, v[0], v[1], v[2])
        {
        }

        explicit constexpr QuaternionHJ(const QuaternionHJ<T, !JPL>& q)
                : data_(q.w(), q.x(), q.y(), q.z())
        {
        }

        [[nodiscard]] static QuaternionHJ<T, JPL> rotation_quaternion(const Vector<3, T>& axis, const T angle)
        {
                return {std::cos(angle / 2), std::sin(angle / 2) * axis.normalized()};
        }

        [[nodiscard]] std::size_t hash() const
        {
                return data_.hash();
        }

        [[nodiscard]] constexpr Vector<3, T> vec() const
        {
                return {x(), y(), z()};
        }

        [[nodiscard]] constexpr T w() const
        {
                return data_[0];
        }

        [[nodiscard]] constexpr T& w()
        {
                return data_[0];
        }

        [[nodiscard]] constexpr T x() const
        {
                return data_[1];
        }

        [[nodiscard]] constexpr T& x()
        {
                return data_[1];
        }

        [[nodiscard]] constexpr T y() const
        {
                return data_[2];
        }

        [[nodiscard]] constexpr T& y()
        {
                return data_[2];
        }

        [[nodiscard]] constexpr T z() const
        {
                return data_[3];
        }

        [[nodiscard]] constexpr T& z()
        {
                return data_[3];
        }

        [[nodiscard]] constexpr QuaternionHJ<T, JPL> conjugate() const
        {
                return {w(), -vec()};
        }

        [[nodiscard]] T norm() const
        {
                return data_.norm();
        }

        [[nodiscard]] QuaternionHJ<T, JPL> normalized() const
        {
                const T norm = data_.norm();
                const T n = (w() >= 0) ? norm : -norm;
                return QuaternionHJ<T, JPL>(data_ / n);
        }

        [[nodiscard]] bool is_unit() const
        {
                return data_.is_unit();
        }

        [[nodiscard]] QuaternionHJ<T, JPL> inversed() const
        {
                return conjugate() / data_.norm_squared();
        }

        [[nodiscard]] friend std::string to_string(const QuaternionHJ<T, JPL>& a)
        {
                std::ostringstream oss;
                oss.precision(Limits<T>::max_digits10());
                oss << '(';
                oss << a.data_[0];
                oss << ", {";
                oss << a.data_[1];
                oss << ", " << a.data_[2];
                oss << ", " << a.data_[3];
                oss << "})";
                return oss.str();
        }

        [[nodiscard]] friend constexpr bool operator==(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
        {
                return a.data_ == b.data_;
        }

        [[nodiscard]] friend constexpr QuaternionHJ<T, JPL> operator+(
                const QuaternionHJ<T, JPL>& a,
                const QuaternionHJ<T, JPL>& b)
        {
                return QuaternionHJ<T, JPL>(a.data_ + b.data_);
        }

        [[nodiscard]] friend constexpr QuaternionHJ<T, JPL> operator-(
                const QuaternionHJ<T, JPL>& a,
                const QuaternionHJ<T, JPL>& b)
        {
                return QuaternionHJ<T, JPL>(a.data_ - b.data_);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr QuaternionHJ<T, JPL> operator*(const QuaternionHJ<T, JPL>& a, const S b)
        {
                return QuaternionHJ<T, JPL>(a.data_ * b);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr QuaternionHJ<T, JPL> operator*(const S b, const QuaternionHJ<T, JPL>& a)
        {
                return QuaternionHJ<T, JPL>(a.data_ * b);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr QuaternionHJ<T, JPL> operator/(const QuaternionHJ<T, JPL>& a, const S b)
        {
                return QuaternionHJ<T, JPL>(a.data_ / b);
        }
};

template <typename T>
using Quaternion = QuaternionHJ<T, false>;
}
