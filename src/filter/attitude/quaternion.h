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

#include <src/numerical/vector.h>

#include <cstddef>
#include <string>
#include <type_traits>

namespace ns::filter::attitude
{
template <typename T>
class Quaternion final
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<4, T> data_;

public:
        constexpr Quaternion()
        {
        }

        constexpr Quaternion(const T x, const T y, const T z, const T w)
                : data_(x, y, z, w)
        {
        }

        constexpr Quaternion(const numerical::Vector<3, T>& v, const T w)
                : data_(v[0], v[1], v[2], w)
        {
        }

        explicit constexpr Quaternion(const numerical::Vector<4, T>& v)
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

        [[nodiscard]] constexpr const numerical::Vector<4, T>& coeffs() const
        {
                return data_;
        }

        [[nodiscard]] std::size_t hash() const
        {
                return data_.hash();
        }

        [[nodiscard]] constexpr numerical::Vector<3, T> vec() const
        {
                return {data_[0], data_[1], data_[2]};
        }

        [[nodiscard]] constexpr Quaternion<T> conjugate() const
        {
                return {-data_[0], -data_[1], -data_[2], data_[3]};
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
                return to_string(a.coeffs());
        }
};

template <typename T>
[[nodiscard]] constexpr bool operator==(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.coeffs() == b.coeffs();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.coeffs() + b.coeffs());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion<T>(a.coeffs() - b.coeffs());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.coeffs() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const T b, const Quaternion<T>& a)
{
        return Quaternion<T>(a.coeffs() * b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator/(const Quaternion<T>& a, const T b)
{
        return Quaternion<T>(a.coeffs() / b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // a[3] * b.vec() + b[3] * a.vec() - cross(a.vec(), b.vec())
        // a[3] * b[3] - dot(a.vec(), b.vec())
        Quaternion<T> res;
        res[0] = a[3] * b[0] + b[3] * a[0] - a[1] * b[2] + a[2] * b[1];
        res[1] = a[3] * b[1] + b[3] * a[1] + a[0] * b[2] - a[2] * b[0];
        res[2] = a[3] * b[2] + b[3] * a[2] - a[0] * b[1] + a[1] * b[0];
        res[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
        return res;
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> quaternion_mul(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        // -cross(a.vec(), b.vec())
        // -dot(a.vec(), b.vec())
        Quaternion<T> res;
        res[0] = a[2] * b[1] - a[1] * b[2];
        res[1] = a[0] * b[2] - a[2] * b[0];
        res[2] = a[1] * b[0] - a[0] * b[1];
        res[3] = -a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
        return res;
}
}
