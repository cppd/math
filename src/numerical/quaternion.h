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
#include "vector.h"

#include <src/com/error.h>

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>

namespace std
{
template <typename T, bool JPL>
struct hash<::ns::numerical::QuaternionHJ<T, JPL>> final
{
        [[nodiscard]] static size_t operator()(const ::ns::numerical::QuaternionHJ<T, JPL>& q)
        {
                return q.hash();
        }
};

template <typename T, bool JPL>
struct tuple_size<::ns::numerical::QuaternionHJ<T, JPL>> final : integral_constant<size_t, 4>
{
};
}

namespace ns::numerical
{
namespace quaternion_implementation
{
template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(
        const QuaternionHJ<T, JPL>& a,
        const QuaternionHJ<T, JPL>& b)
{
        // a.w() * b.vec() + b.w() * a.vec() + cross(a.vec(), b.vec())
        // a.w() * b.w() - dot(a.vec(), b.vec())
        QuaternionHJ<T, JPL> res;
        res.x() = a.w() * b.x() + b.w() * a.x() + a.y() * b.z() - a.z() * b.y();
        res.y() = a.w() * b.y() + b.w() * a.y() - a.x() * b.z() + a.z() * b.x();
        res.z() = a.w() * b.z() + b.w() * a.z() + a.x() * b.y() - a.y() * b.x();
        res.w() = a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z();
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(const QuaternionHJ<T, JPL>& a, const Vector<3, T>& b)
{
        // a.w() * b + cross(a.vec(), b)
        // -dot(a.vec(), b)
        QuaternionHJ<T, JPL> res;
        res.x() = a.w() * b[0] + a.y() * b[2] - a.z() * b[1];
        res.y() = a.w() * b[1] - a.x() * b[2] + a.z() * b[0];
        res.z() = a.w() * b[2] + a.x() * b[1] - a.y() * b[0];
        res.w() = -a.x() * b[0] - a.y() * b[1] - a.z() * b[2];
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> multiply_hamilton(const Vector<3, T>& a, const QuaternionHJ<T, JPL>& b)
{
        // b.w() * a + cross(a, b.vec())
        // -dot(a, b.vec())
        QuaternionHJ<T, JPL> res;
        res.x() = b.w() * a[0] + a[1] * b.z() - a[2] * b.y();
        res.y() = b.w() * a[1] - a[0] * b.z() + a[2] * b.x();
        res.z() = b.w() * a[2] + a[0] * b.y() - a[1] * b.x();
        res.w() = -a[0] * b.x() - a[1] * b.y() - a[2] * b.z();
        return res;
}

template <typename T, bool JPL>
[[nodiscard]] constexpr Vector<3, T> multiply_hamilton_vec(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        // (a * b).vec()
        // a.w() * b.vec() + b.w() * a.vec() + cross(a.vec(), b.vec())
        Vector<3, T> res;
        res[0] = a.w() * b.x() + b.w() * a.x() + a.y() * b.z() - a.z() * b.y();
        res[1] = a.w() * b.y() + b.w() * a.y() - a.x() * b.z() + a.z() * b.x();
        res[2] = a.w() * b.z() + b.w() * a.z() + a.x() * b.y() - a.y() * b.x();
        return res;
}
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const QuaternionHJ<T, JPL>& a, const Vector<3, T>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr QuaternionHJ<T, JPL> operator*(const Vector<3, T>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton(b, a);
        }
        else
        {
                return impl::multiply_hamilton(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] constexpr Vector<3, T> multiply_vec(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b)
{
        namespace impl = quaternion_implementation;
        if constexpr (JPL)
        {
                return impl::multiply_hamilton_vec(b, a);
        }
        else
        {
                return impl::multiply_hamilton_vec(a, b);
        }
}

template <typename T, bool JPL>
[[nodiscard]] Vector<3, T> rotate_vector(const QuaternionHJ<T, JPL>& q_unit, const Vector<3, T>& v)
{
        ASSERT(q_unit.is_unit());

        return multiply_vec(q_unit * v, q_unit.conjugate());
}

//

template <typename T>
struct QuaternionTraits;

template <typename Type, bool P_JPL>
struct QuaternionTraits<QuaternionHJ<Type, P_JPL>> final
{
        static constexpr bool JPL = P_JPL;
        using T = Type;
};

template <typename Quaternion>
using QuaternionType = QuaternionTraits<Quaternion>::T;
}
