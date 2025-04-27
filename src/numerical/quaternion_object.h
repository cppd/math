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

#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>

namespace ns::numerical
{
template <typename T>
class Quaternion final
{
        static_assert(std::is_floating_point_v<T>);

        Vector<4, T> data_;

        explicit constexpr Quaternion(const Vector<4, T>& v)
                : data_(v)
        {
        }

public:
        constexpr Quaternion()
        {
        }

        constexpr Quaternion(const T w, const T x, const T y, const T z)
                : data_(w, x, y, z)
        {
        }

        constexpr Quaternion(const T w, const Vector<3, T>& v)
                : data_(w, v[0], v[1], v[2])
        {
        }

        [[nodiscard]] std::size_t hash() const
        {
                return data_.hash();
        }

        [[nodiscard]] constexpr Vector<3, T> vec() const
        {
                return {data_[1], data_[2], data_[3]};
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

        [[nodiscard]] constexpr Quaternion<T> conjugate() const
        {
                return {w(), -x(), -y(), -z()};
        }

        [[nodiscard]] T norm() const
        {
                return data_.norm();
        }

        [[nodiscard]] Quaternion<T> normalized() const
        {
                const T norm = data_.norm();
                const T n = (w() >= 0) ? norm : -norm;
                return Quaternion<T>(data_ / n);
        }

        [[nodiscard]] bool is_unit() const
        {
                return data_.is_unit();
        }

        [[nodiscard]] Quaternion<T> inversed() const
        {
                return conjugate() / data_.norm_squared();
        }

        [[nodiscard]] friend std::string to_string(const Quaternion<T>& a)
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

        [[nodiscard]] friend constexpr bool operator==(const Quaternion<T>& a, const Quaternion<T>& b)
        {
                return a.data_ == b.data_;
        }

        [[nodiscard]] friend constexpr Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
        {
                return Quaternion<T>(a.data_ + b.data_);
        }

        [[nodiscard]] friend constexpr Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
        {
                return Quaternion<T>(a.data_ - b.data_);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr Quaternion<T> operator*(const Quaternion<T>& a, const S b)
        {
                return Quaternion<T>(a.data_ * b);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr Quaternion<T> operator*(const S b, const Quaternion<T>& a)
        {
                return Quaternion<T>(a.data_ * b);
        }

        template <typename S>
                requires (std::is_same_v<T, S>)
        [[nodiscard]] friend constexpr Quaternion<T> operator/(const Quaternion<T>& a, const S b)
        {
                return Quaternion<T>(a.data_ / b);
        }
};
}
