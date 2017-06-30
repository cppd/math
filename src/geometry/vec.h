/*
Copyright (C) 2017 Topological Manifold

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
        constexpr Vector(std::integer_sequence<size_t, I...>, const T& v) : m_data{{(static_cast<void>(I), v)...}}
        {
                static_assert(sizeof...(I) == N);
        }

public:
        Vector() = default;

        template <typename... Args>
        constexpr Vector(Args... args) : m_data{{static_cast<T>(args)...}}
        {
                static_assert(sizeof...(args) == N);
        }

        template <typename Arg>
        constexpr explicit Vector(Arg v) : Vector(std::make_integer_sequence<size_t, N>(), static_cast<T>(v))
        {
        }

        constexpr T operator[](unsigned i) const
        {
                return m_data[i];
        }

        T& operator[](unsigned i)
        {
                return m_data[i];
        }

        size_t get_hash() const
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
};

namespace std
{
template <size_t N, typename T>
struct hash<Vector<N, T>>
{
        size_t operator()(const Vector<N, T>& v) const
        {
                return v.get_hash();
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
T dot(const Vector<N, T>& a, const Vector<N, T>& b)
{
        T result = a[0] * b[0];
        for (unsigned i = 1; i < N; ++i)
        {
                result = any_fma(a[i], b[i], result);
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

template <typename NEW_TYPE, size_t N, typename T>
Vector<N, NEW_TYPE> to_vec(const Vector<N, T>& v)
{
        Vector<N, NEW_TYPE> result;
        for (unsigned i = 0; i < N; ++i)
        {
                result[i] = v[i];
        }
        return result;
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

// vec - это только Vector<N, double>.
// Не менять.
template <std::size_t N>
using vec = Vector<N, double>;
