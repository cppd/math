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

#include "com/error.h"

// Для случаев, когда последняя строка матрицы состоит из нулей с последней единицей.
template <size_t N, typename T>
class MatrixVectorMultiplier
{
        Matrix<N, N, T> m_matrix;

public:
        explicit MatrixVectorMultiplier(const Matrix<N, N, T>& m) : m_matrix(m)
        {
                if (m_matrix(N - 1, N - 1) != 1)
                {
                        error("Wrong matrix for matrix-vector multiplier");
                }
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (m_matrix(N - 1, i) != 0)
                        {
                                error("Wrong matrix for matrix-vector multiplier");
                        }
                }
        }

        Vector<N - 1, T> operator()(const Vector<N - 1, T>& v) const
        {
                Vector<N - 1, T> res;
                for (unsigned r = 0; r < N - 1; ++r)
                {
                        res[r] = m_matrix(r, 0) * v[0];
                        for (unsigned c = 1; c < N - 1; ++c)
                        {
                                res[r] = fma(m_matrix(r, c), v[c], res[r]);
                        }
                        res[r] += m_matrix(r, N - 1);
                }
                return res;
        }
};
