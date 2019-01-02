/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "vec.h"

#include "com/type/trait.h"

template <typename T>
class Quaternion
{
        static_assert(is_native_floating_point<T>);

        Vector<4, T> m_data;

public:
        Quaternion() = default;

        Quaternion(T w, T x, T y, T z) : m_data(w, x, y, z)
        {
        }

        Quaternion(T w, const Vector<3, T>& v) : m_data(w, v[0], v[1], v[2])
        {
        }

        Quaternion(const Vector<4, T>& v) : m_data(v)
        {
        }

        T operator[](unsigned i) const
        {
                return m_data[i];
        }

        T& operator[](unsigned i)
        {
                return m_data[i];
        }

        const Vector<4, T>& data() const
        {
                return m_data;
        }

        Vector<3, T> imag() const
        {
                return Vector<3, T>(m_data[1], m_data[2], m_data[3]);
        }
};

template <typename T>
Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.data() + b.data();
}

template <typename T>
Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.data() - b.data();
}

template <typename T>
Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        Vector<3, T> a_v = a.imag();
        Vector<3, T> b_v = b.imag();

        return Quaternion<T>(a[0] * b[0] - dot(a_v, b_v), a[0] * b_v + b[0] * a_v + cross(a_v, b_v));
}

template <typename T>
Quaternion<T> operator*(const Quaternion<T>& a, T b)
{
        return a.data() * b;
}

template <typename T>
Quaternion<T> operator/(const Quaternion<T>& a, T b)
{
        return a.data() / b;
}

template <typename T>
Quaternion<T> conjugate(const Quaternion<T>& a)
{
        return Quaternion<T>(a[0], -a[1], -a[2], -a[3]);
}

template <typename T>
Quaternion<T> inverse(const Quaternion<T>& a)
{
        return conjugate(a) / dot(a.data(), a.data());
}

template <typename T>
std::string to_string(const Quaternion<T>& a)
{
        return to_string(a.data());
}

template <typename T>
Quaternion<T> quaternion_for_rotation(const Vector<3, T>& axis, T angle)
{
        return Quaternion<T>(any_cos(angle / 2), any_sin(angle / 2) * normalize(axis));
}

template <typename T>
Vector<3, T> rotate_vector(const Vector<3, T>& axis, T angle, const Vector<3, T>& v)
{
        Quaternion q = quaternion_for_rotation(axis, angle);
        return (q * Quaternion<T>(static_cast<T>(0), v) * conjugate(q)).imag();
}
