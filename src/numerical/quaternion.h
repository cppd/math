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
        Quaternion()
        {
        }

        Quaternion(const T w, const T x, const T y, const T z)
                : data_(w, x, y, z)
        {
        }

        Quaternion(const T w, const Vector<3, T>& v)
                : data_(w, v[0], v[1], v[2])
        {
        }

        explicit Quaternion(const Vector<4, T>& v)
                : data_(v)
        {
        }

        [[nodiscard]] T operator[](const std::size_t i) const
        {
                return data_[i];
        }

        [[nodiscard]] T& operator[](const std::size_t i)
        {
                return data_[i];
        }

        [[nodiscard]] const Vector<4, T>& data() const
        {
                return data_;
        }

        [[nodiscard]] Vector<3, T> imag() const
        {
                return Vector<3, T>(data_[1], data_[2], data_[3]);
        }
};

template <typename T>
[[nodiscard]] Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.data() + b.data();
}

template <typename T>
[[nodiscard]] Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.data() - b.data();
}

template <typename T>
[[nodiscard]] Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        const Vector<3, T> a_v = a.imag();
        const Vector<3, T> b_v = b.imag();

        return Quaternion<T>(a[0] * b[0] - dot(a_v, b_v), a[0] * b_v + b[0] * a_v + cross(a_v, b_v));
}

template <typename T>
[[nodiscard]] Quaternion<T> operator*(const Quaternion<T>& a, const T b)
{
        return a.data() * b;
}

template <typename T>
[[nodiscard]] Quaternion<T> operator/(const Quaternion<T>& a, const T b)
{
        return a.data() / b;
}

template <typename T>
[[nodiscard]] Quaternion<T> conjugate(const Quaternion<T>& a)
{
        return Quaternion<T>(a[0], -a[1], -a[2], -a[3]);
}

template <typename T>
[[nodiscard]] Quaternion<T> inverse(const Quaternion<T>& a)
{
        return conjugate(a) / dot(a.data(), a.data());
}

template <typename T>
[[nodiscard]] std::string to_string(const Quaternion<T>& a)
{
        return to_string(a.data());
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
        return (q * Quaternion<T>(0, v) * conjugate(q)).imag();
}
}
