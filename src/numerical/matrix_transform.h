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

#include "matrix.h"

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

#if defined(OPENGL_FOUND)
template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
constexpr Matrix<4, 4, T> ortho_opengl(T1 left, T2 right, T3 bottom, T4 top, T5 near, T6 far)
{
        // OpenGL
        // X направо [-1, 1], Y вверх [-1, 1], Z в экран [-1, 1]

        T left_t = left;
        T right_t = right;
        T bottom_t = bottom;
        T top_t = top;
        T near_t = near;
        T far_t = far;

        Matrix<4, 4, T> m(1);

        m(0, 0) = 2 / (right_t - left_t);
        m(1, 1) = 2 / (top_t - bottom_t);
        m(2, 2) = 2 / (far_t - near_t);

        m(0, 3) = -(right_t + left_t) / (right_t - left_t);
        m(1, 3) = -(top_t + bottom_t) / (top_t - bottom_t);
        m(2, 3) = -(far_t + near_t) / (far_t - near_t);

        return m;
}
#endif

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
constexpr Matrix<4, 4, T> ortho_vulkan(T1 left, T2 right, T3 bottom, T4 top, T5 near, T6 far)
{
        // Vulkan
        // X направо [-1, 1], Y вниз [-1, 1], Z в экран [0, 1]

        T left_t = left;
        T right_t = right;
        T bottom_t = bottom;
        T top_t = top;
        T near_t = near;
        T far_t = far;

        Matrix<4, 4, T> m(1);

        m(0, 0) = 2 / (right_t - left_t);
        m(1, 1) = 2 / (bottom_t - top_t);
        m(2, 2) = 1 / (far_t - near_t);

        m(0, 3) = -(right_t + left_t) / (right_t - left_t);
        m(1, 3) = -(bottom_t + top_t) / (bottom_t - top_t);
        m(2, 3) = -near_t / (far_t - near_t);

        return m;
}

template <size_t N, typename T>
constexpr Matrix<N + 1, N + 1, T> scale(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> m(1);
        for (unsigned i = 0; i < N; ++i)
        {
                m(i, i) = v[i];
        }
        return m;
}

template <typename T, typename... V>
constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> scale(V... v)
{
        return scale(Vector<sizeof...(V), T>(v...));
}

template <size_t N, typename T>
constexpr Matrix<N + 1, N + 1, T> translate(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> m(1);
        for (unsigned i = 0; i < N; ++i)
        {
                m(i, N) = v[i];
        }
        return m;
}

template <typename T, typename... V>
constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> translate(V... v)
{
        return translate(Vector<sizeof...(V), T>(v...));
}
