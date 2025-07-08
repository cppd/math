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

#include "matrix.h"
#include "rotation.h"
#include "vector.h"

#include <src/com/type/limit.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>

namespace ns::numerical
{
struct IdentityQuaternion final
{
};

inline constexpr IdentityQuaternion IDENTITY_QUATERNION;

template <typename T, bool JPL>
class QuaternionHJ final
{
        friend class QuaternionHJ<T, !JPL>;

        static_assert(std::is_floating_point_v<T>);

        Vector<4, T> data_;

        explicit constexpr QuaternionHJ(const auto& v)
                : data_(v)
        {
        }

public:
        constexpr QuaternionHJ()
        {
        }

        explicit constexpr QuaternionHJ(IdentityQuaternion)
                : QuaternionHJ({0, 0, 0}, 1)
        {
        }

        constexpr QuaternionHJ(const Vector<3, T>& v, const T w)
                : data_(v[0], v[1], v[2], w)
        {
        }

        constexpr QuaternionHJ(const T w, const Vector<3, T>& v)
                : QuaternionHJ(v, w)
        {
        }

        explicit constexpr QuaternionHJ(const QuaternionHJ<T, !JPL>& q)
                : data_(q.data_)
        {
        }

        [[nodiscard]] static QuaternionHJ<T, JPL> rotation_quaternion(const T angle, const Vector<3, T>& axis)
        {
                return rotation_vector_to_quaternion<QuaternionHJ<T, JPL>>(angle, axis);
        }

        [[nodiscard]] static QuaternionHJ<T, JPL> rotation_quaternion(const Matrix<3, 3, T>& rotation_matrix)
        {
                return rotation_matrix_to_quaternion<QuaternionHJ<T, JPL>>(rotation_matrix);
        }

        [[nodiscard]] Matrix<3, 3, T> rotation_matrix() const
        {
                return rotation_quaternion_to_matrix(*this);
        }

        [[nodiscard]] std::size_t hash() const
        {
                return data_.hash();
        }

        [[nodiscard]] constexpr Vector<3, T> vec() const
        {
                return {x(), y(), z()};
        }

        [[nodiscard]] constexpr T x() const
        {
                return data_[0];
        }

        [[nodiscard]] constexpr T& x()
        {
                return data_[0];
        }

        [[nodiscard]] constexpr T y() const
        {
                return data_[1];
        }

        [[nodiscard]] constexpr T& y()
        {
                return data_[1];
        }

        [[nodiscard]] constexpr T z() const
        {
                return data_[2];
        }

        [[nodiscard]] constexpr T& z()
        {
                return data_[2];
        }

        [[nodiscard]] constexpr T w() const
        {
                return data_[3];
        }

        [[nodiscard]] constexpr T& w()
        {
                return data_[3];
        }

        [[nodiscard]] constexpr QuaternionHJ<T, JPL> conjugate() const
        {
                return {-vec(), w()};
        }

        [[nodiscard]] T norm() const
        {
                return data_.norm();
        }

        [[nodiscard]] QuaternionHJ<T, JPL> normalized() const
        {
                const T norm = data_.norm();
                const T n = (w() < 0) ? -norm : norm;
                return QuaternionHJ<T, JPL>(data_ / n);
        }

        [[nodiscard]] QuaternionHJ<T, JPL> inversed() const
        {
                return conjugate() / data_.norm_squared();
        }

        [[nodiscard]] constexpr bool is_unit() const
        {
                return data_.is_unit();
        }

        [[nodiscard]] constexpr bool is_normalized() const
        {
                return (w() >= 0) && is_unit();
        }

        [[nodiscard]] friend constexpr bool is_finite(const QuaternionHJ<T, JPL>& a)
        {
                return is_finite(a.data_);
        }

        [[nodiscard]] friend std::string to_string(const QuaternionHJ<T, JPL>& a)
        {
                std::ostringstream oss;
                oss.precision(Limits<T>::max_digits10());
                oss << "({" << a.x() << ", " << a.y() << ", " << a.z() << "}, " << a.w() << ')';
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
