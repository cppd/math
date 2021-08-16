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

#include "matrix.h"

#include <src/com/error.h>

namespace ns::matrix
{
template <typename T>
Matrix<4, 4, T> look_at(const Vector<3, T>& eye, const Vector<3, T>& center, const Vector<3, T>& up)
{
        Vector<3, T> f = (center - eye).normalized();
        Vector<3, T> s = cross(f, up).normalized();
        Vector<3, T> u = cross(s, f).normalized();

        Matrix<4, 4, T> m;

        m.row(0) = Vector<4, T>(s[0], s[1], s[2], -dot(s, eye));
        m.row(1) = Vector<4, T>(u[0], u[1], u[2], -dot(u, eye));
        m.row(2) = Vector<4, T>(-f[0], -f[1], -f[2], dot(f, eye));
        m.row(3) = Vector<4, T>(0, 0, 0, 1);

        return m;
}

// Right-handed coordinate systems
// Source: X to the right, Y upward
// Vulkan: X to the right [-1, 1], Y downward [-1, 1], Z [0, 1]
template <typename T>
constexpr Matrix<4, 4, T> ortho_vulkan(
        const std::type_identity_t<T>& left,
        const std::type_identity_t<T>& right,
        const std::type_identity_t<T>& bottom,
        const std::type_identity_t<T>& top,
        const std::type_identity_t<T>& near,
        const std::type_identity_t<T>& far)
{
        Matrix<4, 4, T> m(1);

        m(0, 0) = 2 / (right - left);
        m(1, 1) = 2 / (bottom - top);
        m(2, 2) = 1 / (far - near);

        m(0, 3) = -(right + left) / (right - left);
        m(1, 3) = -(bottom + top) / (bottom - top);
        m(2, 3) = -near / (far - near);

        return m;
}

template <std::size_t N, typename T>
constexpr Matrix<N + 1, N + 1, T> scale(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> m(1);
        for (std::size_t i = 0; i < N; ++i)
        {
                m(i, i) = v[i];
        }
        return m;
}

template <typename T, typename... V>
constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> scale(const V&... v)
{
        return scale(Vector<sizeof...(V), T>(v...));
}

template <std::size_t N, typename T>
constexpr Matrix<N + 1, N + 1, T> translate(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> m(1);
        for (std::size_t i = 0; i < N; ++i)
        {
                m(i, N) = v[i];
        }
        return m;
}

template <typename T, typename... V>
constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> translate(const V&... v)
{
        return translate(Vector<sizeof...(V), T>(v...));
}

template <std::size_t N, typename T>
class MatrixVectorMultiplier
{
        Matrix<N, N, T> matrix_;

public:
        explicit MatrixVectorMultiplier(const Matrix<N, N, T>& m) : matrix_(m)
        {
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        if (matrix_(N - 1, i) != 0)
                        {
                                error("Wrong matrix for matrix-vector multiplier");
                        }
                }
                if (matrix_(N - 1, N - 1) != 1)
                {
                        error("Wrong matrix for matrix-vector multiplier");
                }
        }

        Vector<N - 1, T> operator()(const Vector<N - 1, T>& v) const
        {
                Vector<N - 1, T> res;
                for (std::size_t r = 0; r < N - 1; ++r)
                {
                        res[r] = matrix_(r, 0) * v[0];
                        for (std::size_t c = 1; c < N - 1; ++c)
                        {
                                res[r] += matrix_(r, c) * v[c];
                        }
                        res[r] += matrix_(r, N - 1);
                }
                return res;
        }
};
}
