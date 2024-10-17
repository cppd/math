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

#include "vector.h"

#include <cmath>
#include <cstddef>
#include <string>

namespace ns::numerical
{
template <typename T>
class Quaternion final
{
        static_assert(std::is_floating_point_v<T>);

        Vector<4, T> data_;

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

        explicit constexpr Quaternion(const Vector<4, T>& v)
                : data_(v)
        {
        }

        [[nodiscard]] constexpr T operator[](const std::size_t i) const
        {
                return data_[i];
        }

        [[nodiscard]] constexpr T& operator[](const std::size_t i)
        {
                return data_[i];
        }

        [[nodiscard]] constexpr const Vector<4, T>& data() const
        {
                return data_;
        }

        [[nodiscard]] constexpr Vector<3, T> imag() const
        {
                return Vector<3, T>(data_[1], data_[2], data_[3]);
        }

        [[nodiscard]] constexpr Quaternion<T> conjugate() const
        {
                return Quaternion<T>(data_[0], -data_[1], -data_[2], -data_[3]);
        }

        void normalize()
        {
                data_.normalize();
        }

        [[nodiscard]] Quaternion<T> normalized() const
        {
                return Quaternion<T>(data_.normalized());
        }

        [[nodiscard]] Quaternion<T> inversed() const
        {
                return conjugate() / data_.norm_squared();
        }

        [[nodiscard]] friend std::string to_string(const Quaternion<T>& a)
        {
                return to_string(a.data());
        }
};

template <typename T>
[[nodiscard]] constexpr bool operator==(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.data() == b.data();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.data() + b.data());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.data() - b.data());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.data() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const T b, const Quaternion<T>& a)
{
        return Quaternion<T>(a.data() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator/(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.data() / b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // a[0] * b[0] - dot(a.imag(), b.imag())
        // a[0] * b.imag() + b[0] * a.imag() + cross(a.imag(), b.imag())
        Quaternion<T> res;
        res[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
        res[1] = a[0] * b[1] + b[0] * a[1] + a[2] * b[3] - a[3] * b[2];
        res[2] = a[0] * b[2] + b[0] * a[2] - a[1] * b[3] + a[3] * b[1];
        res[3] = a[0] * b[3] + b[0] * a[3] + a[1] * b[2] - a[2] * b[1];
        return res;
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Vector<3, T>& b)
{
        // -dot(a.imag(), b)
        // a[0] * b + cross(a.imag(), b)
        Quaternion<T> res;
        res[0] = -a[1] * b[0] - a[2] * b[1] - a[3] * b[2];
        res[1] = a[0] * b[0] + a[2] * b[2] - a[3] * b[1];
        res[2] = a[0] * b[1] - a[1] * b[2] + a[3] * b[0];
        res[3] = a[0] * b[2] + a[1] * b[1] - a[2] * b[0];
        return res;
}

template <typename T>
[[nodiscard]] Quaternion<T> quaternion_for_rotation(const Vector<3, T>& axis, const T angle)
{
        return Quaternion<T>(std::cos(angle / 2), std::sin(angle / 2) * axis.normalized());
}

template <typename T>
[[nodiscard]] Vector<3, T> rotate_vector(const Vector<3, T>& axis, const T angle, const Vector<3, T>& v)
{
        const Quaternion q = quaternion_for_rotation(axis, angle);
        return (q * v * q.conjugate()).imag();
}
}
