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
