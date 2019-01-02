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

#include "com/hash.h"
#include "com/math.h"
#include "com/print.h"

#include <array>
#include <cmath>

template <size_t N, typename T>
class Vector
{
        static_assert(N > 0);

        std::array<T, N> m_data;

        template <size_t... I>
        constexpr Vector(std::integer_sequence<size_t, I...>&&, const T& v) : m_data{(static_cast<void>(I), v)...}
        {
                static_assert(sizeof...(I) == N);
        }

public:
        Vector() = default;

        template <typename Arg1, typename Arg2, typename... Args>
        constexpr Vector(Arg1&& arg1, Arg2&& arg2, Args&&... args)
                : m_data{static_cast<T>(std::forward<Arg1>(arg1)), static_cast<T>(std::forward<Arg2>(arg2)),
                         static_cast<T>(std::forward<Args>(args))...}
        {
                static_assert(sizeof...(args) + 2 == N);
        }

        constexpr explicit Vector(const T& v) : Vector(std::make_integer_sequence<size_t, N>(), v)
        {
        }

        constexpr const T& operator[](unsigned i) const
        {
                return m_data[i];
        }

        constexpr T& operator[](unsigned i)
        {
                return m_data[i];
        }

        size_t hash() const
        {
                return array_hash(m_data);
        }

        constexpr bool operator==(const Vector<N, T>& a) const
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (m_data[i] != a.m_data[i])
                        {
                                return false;
                        }
                }
                return true;
        }

        Vector<N, T>& operator+=(const Vector<N, T>& a)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        m_data[i] += a[i];
                }
                return *this;
        }

        Vector<N, T>& operator-=(const Vector<N, T>& a)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        m_data[i] -= a[i];
                }
                return *this;
        }

        Vector<N, T>& operator*=(const T& v)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        m_data[i] *= v;
                }
                return *this;
        }

        Vector<N, T>& operator/=(const T& v)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        m_data[i] /= v;
                }
                return *this;
        }

        const T* data() const
        {
                static_assert(sizeof(Vector) == N * sizeof(T));
                return m_data.data();
        }
};

namespace std
{
template <size_t N, typename T>
struct hash<Vector<N, T>>
{
        size_t operator()(const Vector<N, T>& v) const
        {
                return v.hash();
        }
};
}

template <size_t N, typename T>
Vector<N, T> operator+(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = a[i] + b[i];
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator-(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = a[i] - b[i];
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator*(const Vector<N, T>& a, T b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = a[i] * b;
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator*(T b, const Vector<N, T>& a)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = b * a[i];
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator*(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = a[i] * b[i];
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator/(const Vector<N, T>& a, T b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = a[i] / b;
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> operator-(const Vector<N, T>& a)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = -a[i];
        }
        return res;
}

template <size_t N, typename T>
T max_element(const Vector<N, T>& a)
{
        T max = a[0];
        for (unsigned i = 1; i < N; ++i)
        {
                max = std::max(a[i], max);
        }
        return max;
}

template <size_t N, typename T>
T min_element(const Vector<N, T>& a)
{
        T min = a[0];
        for (unsigned i = 1; i < N; ++i)
        {
                min = std::min(a[i], min);
        }
        return min;
}

template <size_t N, typename T>
Vector<N, T> max_vector(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = std::max(a[i], b[i]);
        }
        return res;
}

template <size_t N, typename T>
Vector<N, T> min_vector(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = std::min(a[i], b[i]);
        }
        return res;
}

template <size_t N, typename T>
T dot(const Vector<N, T>& a, const Vector<N, T>& b)
{
        T result = a[0] * b[0];
        for (unsigned i = 1; i < N; ++i)
        {
                result = any_fma(a[i], b[i], result);
        }
        return result;
}

template <size_t N, typename T, typename F>
Vector<N, T> interpolation(const Vector<N, T>& a, const Vector<N, T>& b, F x)
{
        Vector<N, T> result;
        for (unsigned i = 0; i < N; ++i)
        {
                result[i] = interpolation(a[i], b[i], x);
        }
        return result;
}

template <size_t N, typename T>
T length(const Vector<N, T>& a)
{
        return any_sqrt(dot(a, a));
}

template <size_t N, typename T>
Vector<N, T> normalize(const Vector<N, T>& a)
{
        return a / length(a);
}
// template <typename T>
// Vector<1, T> normalize(const Vector<1, T>&)
//{
//        return Vector<1, T>(1);
//}
// template <typename T>
// Vector<2, T> normalize(const Vector<2, T>& a)
//{
//        T k = sqrt_any_type(a[0] * a[0] + a[1] * a[1]);
//        return Vector<2, T>(a[0] / k, a[1] / k);
//}
// template <typename T>
// Vector<3, T> normalize(const Vector<3, T>& a)
//{
//        T k = sqrt_any_type(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
//        return Vector<3, T>(a[0] / k, a[1] / k, a[2] / k);
//}
// template <typename T>
// Vector<4, T> normalize(const Vector<4, T>& a)
//{
//        T k = sqrt_any_type(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
//        return Vector<4, T>(a[0] / k, a[1] / k, a[2] / k, a[3] / k);
//}

template <size_t N, typename T>
Vector<N, T> normalize_mul(const Vector<N, T>& a)
{
        return a * (1 / length(a));
}

template <size_t N, typename T>
bool is_finite(const Vector<N, T>& data)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!is_finite(data[i]))
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, typename T>
bool zero_vector(const Vector<N, T>& v)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (v[i] != 0)
                {
                        return false;
                }
        }
        return true;
}

namespace vector_implementation
{
template <typename Dst, size_t N, typename Src, size_t... I>
Vector<N, Dst> convert_vector(const Vector<N, Src>& v, std::integer_sequence<size_t, I...>)
{
        static_assert(sizeof...(I) == N);
        static_assert(((I < N) && ...));
        static_assert(!std::is_same_v<Dst, Src>);

        return {v[I]...};
}
}

template <typename Dst, size_t N, typename Src>
std::enable_if_t<!std::is_same_v<Dst, Src>, Vector<N, Dst>> to_vector(const Vector<N, Src>& v)
{
        return vector_implementation::convert_vector<Dst>(v, std::make_integer_sequence<size_t, N>());
}

template <typename Dst, size_t N, typename Src>
std::enable_if_t<std::is_same_v<Dst, Src>, const Vector<N, Dst>&> to_vector(const Vector<N, Src>& v)
{
        return v;
}

template <typename Dst, size_t N, typename Src>
std::enable_if_t<!std::is_same_v<Dst, Src>, std::vector<Vector<N, Dst>>> to_vector(const std::vector<Vector<N, Src>>& v)
{
        std::vector<Vector<N, Dst>> res(v.size());
        for (unsigned i = 0; i < v.size(); ++i)
        {
                res[i] = to_vector<Dst>(v[i]);
        }
        return res;
}

template <typename Dst, size_t N, typename Src>
std::enable_if_t<std::is_same_v<Dst, Src>, const std::vector<Vector<N, Dst>>&> to_vector(const std::vector<Vector<N, Src>>& v)
{
        return v;
}

template <size_t N, typename T>
std::string to_string(const Vector<N, T>& data)
{
        std::string o;
        o += "(";
        for (size_t i = 0; i < N; ++i)
        {
                o += to_string(data[i]);
                if (i != N - 1)
                {
                        o += ", ";
                }
        }
        o += ")";
        return o;
}

// Если векторы единичные, то это синус угла между векторами
// в двухмерном пространстве
template <typename T>
T cross(const Vector<2, T>& v0, const Vector<2, T>& v1)
{
        return v0[0] * v1[1] - v0[1] * v1[0];
}

// Дублирование кода из функции ortho_nn, но так удобнее, так как понятие
// векторное произведение имеется только в трёхмерном пространстве,
// в отличие от ортогональных дополнений
template <typename T>
Vector<3, T> cross(const Vector<3, T>& v0, const Vector<3, T>& v1)
{
        Vector<3, T> res;

        // clang-format off

        res[0] = +(v0[1] * v1[2] - v0[2] * v1[1]);
        res[1] = -(v0[0] * v1[2] - v0[2] * v1[0]);
        res[2] = +(v0[0] * v1[1] - v0[1] * v1[0]);

        // clang-format on

        return res;
}

// vec - это только Vector<N, double>.
// Не менять.
template <std::size_t N>
using vec = Vector<N, double>;

// Не менять эти типы.
using vec2 = Vector<2, double>;
using vec3 = Vector<3, double>;
using vec4 = Vector<4, double>;
using vec2f = Vector<2, float>;
using vec3f = Vector<3, float>;
using vec4f = Vector<4, float>;
using vec2i = Vector<2, int>;
using vec3i = Vector<3, int>;
using vec4i = Vector<4, int>;
