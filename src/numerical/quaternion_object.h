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
#include <type_traits>

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

        [[nodiscard]] constexpr const Vector<4, T>& coeffs() const
        {
                return data_;
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

        [[nodiscard]] constexpr T x() const
        {
                return data_[1];
        }

        [[nodiscard]] constexpr T y() const
        {
                return data_[2];
        }

        [[nodiscard]] constexpr T z() const
        {
                return data_[3];
        }

        [[nodiscard]] constexpr Quaternion<T> conjugate() const
        {
                return {data_[0], -data_[1], -data_[2], -data_[3]};
        }

        [[nodiscard]] T norm() const
        {
                return data_.norm();
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
}
