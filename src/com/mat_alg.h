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

#include "error.h"
#include "mat.h"

// Для случаев, когда последняя строка матрицы состоит из нулей с последней единицей.
template <typename T>
class MatrixMulVector
{
        Matrix<4, 4, T> m_mtx;

public:
        MatrixMulVector(const Matrix<4, 4, T>& m) : m_mtx(m)
        {
                if (!(m_mtx[3][0] == 0 && m_mtx[3][1] == 0 && m_mtx[3][2] == 0 && m_mtx[3][3] == 1))
                {
                        error("Wrong matrix for matrix-vector multiplier");
                }
        }

        Vector<3, T> operator()(const Vector<3, T>& v)
        {
                return vec3(any_fma(m_mtx[0][0], v[0], any_fma(m_mtx[0][1], v[1], any_fma(m_mtx[0][2], v[2], m_mtx[0][3]))),
                            any_fma(m_mtx[1][0], v[0], any_fma(m_mtx[1][1], v[1], any_fma(m_mtx[1][2], v[2], m_mtx[1][3]))),
                            any_fma(m_mtx[2][0], v[0], any_fma(m_mtx[2][1], v[1], any_fma(m_mtx[2][2], v[2], m_mtx[2][3]))));
        }
};

template <typename T>
Matrix<4, 4, T> look_at(const Vector<3, T>& eye, const Vector<3, T>& center, const Vector<3, T>& up)
{
        Vector<3, T> f = normalize(center - eye);
        Vector<3, T> s = normalize(cross(f, up));
        Vector<3, T> u = normalize(cross(s, f));

        Matrix<4, 4, T> m;

        m[0] = Vector<4, T>(s[0], s[1], s[2], -dot(s, eye));
        m[1] = Vector<4, T>(u[0], u[1], u[2], -dot(u, eye));
        m[2] = Vector<4, T>(-f[0], -f[1], -f[2], dot(f, eye));
        m[3] = Vector<4, T>(0, 0, 0, 1);

        return m;
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Matrix<4, 4, T> ortho(T1 left, T2 right, T3 bottom, T4 top, T5 near, T6 far)
{
        T left_t = left;
        T right_t = right;
        T bottom_t = bottom;
        T top_t = top;
        T near_t = near;
        T far_t = far;

        Matrix<4, 4, T> m(1);

        m[0][0] = 2 / (right_t - left_t);
        m[1][1] = 2 / (top_t - bottom_t);
        m[2][2] = -2 / (far_t - near_t);

        m[0][3] = -(right_t + left_t) / (right_t - left_t);
        m[1][3] = -(top_t + bottom_t) / (top_t - bottom_t);
        m[2][3] = -(far_t + near_t) / (far_t - near_t);

        return m;
}

template <typename T>
Matrix<4, 4, T> scale(const Vector<3, T>& v)
{
        Matrix<4, 4, T> m(1);
        m[0][0] = v[0];
        m[1][1] = v[1];
        m[2][2] = v[2];
        return m;
}

template <typename T, typename T0, typename T1, typename T2>
Matrix<4, 4, T> scale(T0 v0, T1 v1, T2 v2)
{
        return scale(Vector<3, T>(v0, v1, v2));
}

template <typename T>
Matrix<4, 4, T> translate(const Vector<3, T>& v)
{
        Matrix<4, 4, T> m(1);
        m[0][3] = v[0];
        m[1][3] = v[1];
        m[2][3] = v[2];
        return m;
}

template <typename T, typename T0, typename T1, typename T2>
Matrix<4, 4, T> translate(T0 v0, T1 v1, T2 v2)
{
        return translate(Vector<3, T>(v0, v1, v2));
}
