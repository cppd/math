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

#include "vector_object.h" // IWYU pragma: export

#include <src/com/interpolation.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

namespace std
{
template <size_t N, typename T>
struct hash<::ns::Vector<N, T>> final
{
        size_t operator()(const ::ns::Vector<N, T>& v) const
        {
                return v.hash();
        }
};

template <size_t N, typename T>
struct tuple_size<::ns::Vector<N, T>> final : integral_constant<size_t, N>
{
};
}

namespace ns
{
template <std::size_t N, typename T>
[[nodiscard]] constexpr bool operator==(const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(a[i] == b[i]))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator+(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] + b[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator-(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] - b[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator*(const Vector<N, T>& a, const T& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] * b;
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator*(const T& b, const Vector<N, T>& a)
{
        return a * b;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator*(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] * b[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator/(const Vector<N, T>& a, const T& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] / b;
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> operator/(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] / b[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> max(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::max(a[i], b[i]);
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> min(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::min(a[i], b[i]);
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr T dot(const Vector<N, T>& a, const Vector<N, T>& b)
{
        T res = a[0] * b[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                res += a[i] * b[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> interpolation(const Vector<N, T>& a, const Vector<N, T>& b, const T& t)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = interpolation(a[i], b[i], t);
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] decltype(auto) is_finite(const Vector<N, T>& v)
{
        return v.is_finite();
}

template <typename Dst, std::size_t N, typename Src>
        requires (!std::is_same_v<Dst, Src>)
[[nodiscard]] constexpr Vector<N, Dst> to_vector(const Vector<N, Src>& v)
{
        Vector<N, Dst> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = v[i];
        }
        return res;
}

template <typename Dst, std::size_t N, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] constexpr decltype(auto) to_vector(const Vector<N, Src>& v)
{
        return v;
}

template <typename Dst, std::size_t N, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] constexpr decltype(auto) to_vector(Vector<N, Src>&& v)
{
        return std::move(v);
}

template <typename Dst, std::size_t N, typename Src>
[[nodiscard]] constexpr Vector<N, Dst> to_vector(const std::array<Src, N>& array)
{
        Vector<N, Dst> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = array[i];
        }
        return res;
}

template <typename Dst, std::size_t N, typename Src>
        requires (!std::is_same_v<Dst, Src>)
[[nodiscard]] std::vector<Vector<N, Dst>> to_vector(const std::vector<Vector<N, Src>>& v)
{
        std::vector<Vector<N, Dst>> res;
        res.reserve(v.size());
        for (const Vector<N, Src>& src : v)
        {
                res.push_back(to_vector<Dst>(src));
        }
        return res;
}

template <typename Dst, std::size_t N, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_vector(const std::vector<Vector<N, Src>>& v)
{
        return v;
}

template <typename Dst, std::size_t N, typename Src>
        requires (std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_vector(std::vector<Vector<N, Src>>&& v)
{
        return std::move(v);
}

template <std::size_t N, typename T>
[[nodiscard]] decltype(auto) to_string(const Vector<N, T>& v)
{
        return v.to_string();
}

template <typename T>
[[nodiscard]] constexpr T cross(const Vector<2, T>& v0, const Vector<2, T>& v1)
{
        return v0[0] * v1[1] - v0[1] * v1[0];
}

template <typename T>
[[nodiscard]] constexpr Vector<3, T> cross(const Vector<3, T>& v0, const Vector<3, T>& v1)
{
        Vector<3, T> res;
        res[0] = v0[1] * v1[2] - v0[2] * v1[1];
        res[1] = v0[2] * v1[0] - v0[0] * v1[2];
        res[2] = v0[0] * v1[1] - v0[1] * v1[0];
        return res;
}

using Vector2d = Vector<2, double>;
using Vector3d = Vector<3, double>;
using Vector4d = Vector<4, double>;
using Vector2f = Vector<2, float>;
using Vector3f = Vector<3, float>;
using Vector4f = Vector<4, float>;
using Vector2i = Vector<2, int>;
using Vector3i = Vector<3, int>;
using Vector4i = Vector<4, int>;
}
