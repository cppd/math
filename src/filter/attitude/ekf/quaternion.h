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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <string>
#include <type_traits>

namespace ns::filter::attitude::ekf
{
template <typename T>
class Quaternion final
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Quaternion<T> q_;

public:
        constexpr Quaternion()
        {
        }

        explicit constexpr Quaternion(const numerical::Quaternion<T>& q)
                : q_(q)
        {
        }

        constexpr Quaternion(const T w, const numerical::Vector<3, T>& v)
                : q_(w, v)
        {
        }

        const numerical::Quaternion<T>& q() const
        {
                return q_;
        }

        [[nodiscard]] constexpr numerical::Vector<3, T> vec() const
        {
                return q_.vec();
        }

        [[nodiscard]] constexpr T w() const
        {
                return q_.w();
        }

        [[nodiscard]] constexpr T& w()
        {
                return q_.w();
        }

        [[nodiscard]] constexpr T x() const
        {
                return q_.x();
        }

        [[nodiscard]] constexpr T& x()
        {
                return q_.x();
        }

        [[nodiscard]] constexpr T y() const
        {
                return q_.y();
        }

        [[nodiscard]] constexpr T& y()
        {
                return q_.y();
        }

        [[nodiscard]] constexpr T z() const
        {
                return q_.z();
        }

        [[nodiscard]] constexpr T& z()
        {
                return q_.z();
        }

        [[nodiscard]] constexpr Quaternion<T> conjugate() const
        {
                return Quaternion(q_.conjugate());
        }

        void normalize()
        {
                q_.normalize();
        }

        [[nodiscard]] Quaternion<T> normalized() const
        {
                return Quaternion(q_.normalized());
        }

        [[nodiscard]] Quaternion<T> inversed() const
        {
                return Quaternion(q_.inversed());
        }

        [[nodiscard]] friend std::string to_string(const Quaternion<T>& a)
        {
                return to_string(a.q_);
        }
};

template <typename T>
[[nodiscard]] constexpr bool operator==(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return a.q() == b.q();
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator+(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion(a.q() + b.q());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator-(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion(a.q() - b.q());
}

template <typename T, typename S>
        requires (std::is_same_v<T, S>)
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const S b)
{
        return Quaternion(a.q() * b);
}

template <typename T, typename S>
        requires (std::is_same_v<T, S>)
[[nodiscard]] constexpr Quaternion<T> operator*(const S b, const Quaternion<T>& a)
{
        return Quaternion(a.q() * b);
}

template <typename T, typename S>
        requires (std::is_same_v<T, S>)
[[nodiscard]] constexpr Quaternion<T> operator/(const Quaternion<T>& a, const S b)
{
        return Quaternion(a.q() / b);
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const Quaternion<T>& b)
{
        return Quaternion(b.q() * a.q());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& a, const numerical::Vector<3, T>& b)
{
        return Quaternion(b * a.q());
}

template <typename T>
[[nodiscard]] constexpr Quaternion<T> operator*(const numerical::Vector<3, T>& a, const Quaternion<T>& b)
{
        return Quaternion(b.q() * a);
}
}
