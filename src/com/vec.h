/*
Copyright (C) 2017-2020 Topological Manifold

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
#include "com/interpolation.h"
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

        const T* data() const
        {
                static_assert(sizeof(Vector) == N * sizeof(T));
                return m_data.data();
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

        // Steven J. Leon.
        // Linear Algebra with Applications. Ninth Edition.
        // Pearson Education, 2015.
        // 5.4 Inner Product Spaces.
        T norm_1() const
        {
                T sum = std::abs(m_data[0]);
                for (unsigned i = 1; i < N; ++i)
                {
                        sum += std::abs(m_data[i]);
                }
                return sum;
        }
        T norm_infinity() const
        {
                T max = std::abs(m_data[0]);
                for (unsigned i = 1; i < N; ++i)
                {
                        max = std::max(std::abs(m_data[i]), max);
                }
                return max;
        }
        T norm_squared() const
        {
                T s = m_data[0] * m_data[0];
                for (unsigned i = 1; i < N; ++i)
                {
                        s = std::fma(m_data[i], m_data[i], s);
                }
                return s;
        }
        T norm() const
        {
                return std::sqrt(norm_squared());
        }
        T norm_stable() const
        {
                const T max = norm_infinity();

                T k = m_data[0] / max;
                T s = k * k;
                for (unsigned i = 1; i < N; ++i)
                {
                        k = m_data[i] / max;
                        s = std::fma(k, k, s);
                }
                return max * std::sqrt(s);
        }

        void normalize()
        {
                T n = norm();
                for (unsigned i = 0; i < N; ++i)
                {
                        m_data[i] /= n;
                }
        }
        [[nodiscard]] Vector<N, T> normalized() const
        {
                T n = norm();
                Vector<N, T> res;
                for (unsigned i = 0; i < N; ++i)
                {
                        res[i] = m_data[i] / n;
                }
                return res;
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
                result = fma(a[i], b[i], result);
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
bool is_finite(const Vector<N, T>& data)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (is_finite(data[i]))
                {
                        continue;
                }
                return false;
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
