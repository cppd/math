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

#include "quaternion_object.h" // IWYU pragma: export

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <type_traits>

namespace ns::filter::attitude::kalman
{
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

template <typename T>
[[nodiscard]] numerical::Vector<3, T> global_to_local(
        const Quaternion<T>& q_unit,
        const numerical::Vector<3, T>& global)
{
        return numerical::rotate_vector(q_unit.q().conjugate(), global);
}
}
