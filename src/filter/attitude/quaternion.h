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

        explicit constexpr Quaternion(const numerical::Vector<4, T>& v)
                : data_(v)
        {
        }

        constexpr Quaternion(const T w, const T x, const T y, const T z)
                : data_(w, x, y, z)
        {
        }

public:
        constexpr Quaternion()
        {
        }

        constexpr Quaternion(const T w, const numerical::Vector<3, T>& v)
                : data_(w, v[0], v[1], v[2])
        {
        }

        [[nodiscard]] std::size_t hash() const
        {
                return data_.hash();
        }

        [[nodiscard]] constexpr numerical::Vector<3, T> vec() const
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
                return to_string(a.data_);
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

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // a.w() * b.w() - dot(a.vec(), b.vec())
        // a.w() * b.vec() + b.w() * a.vec() - cross(a.vec(), b.vec())
        Quaternion<T> res;
        res.w() = a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z();
        res.x() = a.w() * b.x() + b.w() * a.x() - a.y() * b.z() + a.z() * b.y();
        res.y() = a.w() * b.y() + b.w() * a.y() + a.x() * b.z() - a.z() * b.x();
        res.z() = a.w() * b.z() + b.w() * a.z() - a.x() * b.y() + a.y() * b.x();
        return res;
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const numerical::Vector<3, T>& b)
{
        // -dot(a.vec(), b)
        // a.w() * b - cross(a.vec(), b)
        Quaternion<T> res;
        res.w() = -a.x() * b[0] - a.y() * b[1] - a.z() * b[2];
        res.x() = a.w() * b[0] - a.y() * b[2] + a.z() * b[1];
        res.y() = a.w() * b[1] + a.x() * b[2] - a.z() * b[0];
        res.z() = a.w() * b[2] - a.x() * b[1] + a.y() * b[0];
        return res;
}

template <typename T>
[[nodiscard]] constexpr numerical::Vector<3, T> multiply_vec(const Quaternion<T>& a, const Quaternion<T>& b)
{
        // (a * b).vec()
        // a.w() * b.vec() + b.w() * a.vec() - cross(a.vec(), b.vec())
        numerical::Vector<3, T> res;
        res[0] = a.w() * b.x() + b.w() * a.x() - a.y() * b.z() + a.z() * b.y();
        res[1] = a.w() * b.y() + b.w() * a.y() + a.x() * b.z() - a.z() * b.x();
        res[2] = a.w() * b.z() + b.w() * a.z() - a.x() * b.y() + a.y() * b.x();
        return res;
}

template <typename T>
[[nodiscard]] numerical::Vector<3, T> rotate_vector(const Quaternion<T>& q_unit, const numerical::Vector<3, T>& v)
{
        return multiply_vec(q_unit * v, q_unit.conjugate());
}
}
