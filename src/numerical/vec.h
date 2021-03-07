/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/hash.h>
#include <src/com/interpolation.h>
#include <src/com/math.h>
#include <src/com/print.h>

#include <array>
#include <cmath>
#include <functional>
#include <tuple>

namespace ns
{
template <std::size_t N, typename T>
class Vector final
{
        static_assert(N > 0);

        std::array<T, N> m_data;

        template <std::size_t... I>
        constexpr Vector(std::integer_sequence<std::size_t, I...>&&, const T& v) : m_data{(static_cast<void>(I), v)...}
        {
                static_assert(sizeof...(I) == N);
        }

public:
        constexpr Vector()
        {
        }

        template <typename Arg1, typename Arg2, typename... Args>
        constexpr Vector(Arg1&& arg1, Arg2&& arg2, Args&&... args)
                : m_data{
                        static_cast<T>(std::forward<Arg1>(arg1)), static_cast<T>(std::forward<Arg2>(arg2)),
                        static_cast<T>(std::forward<Args>(args))...}
        {
                static_assert(sizeof...(args) + 2 == N);
        }

        constexpr explicit Vector(const T& v) : Vector(std::make_integer_sequence<std::size_t, N>(), v)
        {
        }

        constexpr const T& operator[](std::size_t i) const
        {
                return m_data[i];
        }

        constexpr T& operator[](std::size_t i)
        {
                return m_data[i];
        }

        const T* data() const
        {
                static_assert(sizeof(Vector) == N * sizeof(T));
                return m_data.data();
        }

        std::size_t hash() const
        {
                return array_hash(m_data);
        }

        constexpr bool operator==(const Vector<N, T>& a) const
        {
                for (std::size_t i = 0; i < N; ++i)
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
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] += a[i];
                }
                return *this;
        }

        Vector<N, T>& operator-=(const Vector<N, T>& a)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] -= a[i];
                }
                return *this;
        }

        Vector<N, T>& operator*=(const T& v)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] *= v;
                }
                return *this;
        }

        Vector<N, T>& operator/=(const T& v)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] /= v;
                }
                return *this;
        }

        // Steven J. Leon.
        // Linear Algebra with Applications. Ninth Edition.
        // Pearson Education, 2015.
        // 5.4 Inner Product Spaces.
        [[nodiscard]] T norm_1() const
        {
                T sum = std::abs(m_data[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        sum += std::abs(m_data[i]);
                }
                return sum;
        }
        [[nodiscard]] T norm_infinity() const
        {
                T max = std::abs(m_data[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        max = std::max(std::abs(m_data[i]), max);
                }
                return max;
        }
        [[nodiscard]] T norm_squared() const
        {
                T s = m_data[0] * m_data[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        s = std::fma(m_data[i], m_data[i], s);
                }
                return s;
        }
        [[nodiscard]] T norm() const
        {
                return std::sqrt(norm_squared());
        }
        [[nodiscard]] T norm_stable() const
        {
                const T max = norm_infinity();

                T k = m_data[0] / max;
                T s = k * k;
                for (std::size_t i = 1; i < N; ++i)
                {
                        k = m_data[i] / max;
                        s = std::fma(k, k, s);
                }
                return max * std::sqrt(s);
        }

        void normalize()
        {
                T n = norm();
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] /= n;
                }
        }
        [[nodiscard]] Vector<N, T> normalized() const
        {
                T n = norm();
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = m_data[i] / n;
                }
                return res;
        }

        bool is_zero() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (m_data[i] == 0)
                        {
                                continue;
                        }
                        return false;
                }
                return true;
        }
};

template <std::size_t N, typename T>
Vector<N, T> operator+(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] + b[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator-(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] - b[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator*(const Vector<N, T>& a, T b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] * b;
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator*(T b, const Vector<N, T>& a)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = b * a[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator*(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] * b[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator/(const Vector<N, T>& a, T b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = a[i] / b;
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> operator-(const Vector<N, T>& a)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = -a[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> max_vector(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::max(a[i], b[i]);
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> min_vector(const Vector<N, T>& a, const Vector<N, T>& b)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::min(a[i], b[i]);
        }
        return res;
}

template <std::size_t N, typename T>
T dot(const Vector<N, T>& a, const Vector<N, T>& b)
{
        T res = a[0] * b[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                res = std::fma(a[i], b[i], res);
        }
        return res;
}

template <std::size_t N, typename T, typename F>
Vector<N, T> interpolation(const Vector<N, T>& a, const Vector<N, T>& b, F x)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = interpolation(a[i], b[i], x);
        }
        return res;
}

template <std::size_t N, typename T>
bool is_finite(const Vector<N, T>& data)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (is_finite(data[i]))
                {
                        continue;
                }
                return false;
        }
        return true;
}

template <typename Dst, std::size_t N, typename Src>
std::enable_if_t<!std::is_same_v<Dst, Src>, Vector<N, Dst>> to_vector(const Vector<N, Src>& v)
{
        Vector<N, Dst> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = v[i];
        }
        return res;
}

template <typename Dst, std::size_t N, typename Src>
std::enable_if_t<std::is_same_v<Dst, Src>, const Vector<N, Dst>&> to_vector(const Vector<N, Src>& v)
{
        return v;
}

template <typename Dst, std::size_t N, typename Src>
Vector<N, Dst> to_vector(const std::array<Src, N>& array)
{
        Vector<N, Dst> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = array[i];
        }
        return res;
}

template <typename Dst, std::size_t N, typename Src>
std::enable_if_t<!std::is_same_v<Dst, Src>, std::vector<Vector<N, Dst>>> to_vector(const std::vector<Vector<N, Src>>& v)
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
std::enable_if_t<std::is_same_v<Dst, Src>, const std::vector<Vector<N, Dst>>&> to_vector(
        const std::vector<Vector<N, Src>>& v)
{
        return v;
}

template <std::size_t N, typename T>
std::string to_string(const Vector<N, T>& data)
{
        std::string s;
        s += "(";
        s += to_string(data[0]);
        for (std::size_t i = 1; i < N; ++i)
        {
                s += ", ";
                s += to_string(data[i]);
        }
        s += ")";
        return s;
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
}

namespace std
{
template <size_t N, typename T>
struct hash<::ns::Vector<N, T>>
{
        size_t operator()(const ::ns::Vector<N, T>& v) const
        {
                return v.hash();
        }
};

template <size_t N, typename T>
struct tuple_size<::ns::Vector<N, T>> : integral_constant<size_t, N>
{
};
}
