/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/matrix.h>

#include <cstddef>

namespace ns::filter::attitude::determination
{
template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> adjoint_symmetric(const numerical::Matrix<3, 3, T>& m)
{
        numerical::Matrix<3, 3, T> res;

        res[0, 0] = +(m[1, 1] * m[2, 2] - m[1, 2] * m[2, 1]);
        res[0, 1] = -(m[0, 1] * m[2, 2] - m[0, 2] * m[2, 1]);
        res[0, 2] = +(m[0, 1] * m[1, 2] - m[0, 2] * m[1, 1]);

        res[1, 0] = res[0, 1];
        res[1, 1] = +(m[0, 0] * m[2, 2] - m[0, 2] * m[2, 0]);
        res[1, 2] = -(m[0, 0] * m[1, 2] - m[0, 2] * m[1, 0]);

        res[2, 0] = res[0, 2];
        res[2, 1] = res[1, 2];
        res[2, 2] = +(m[0, 0] * m[1, 1] - m[0, 1] * m[1, 0]);

        return res;
}

template <typename T>
[[nodiscard]] T determinant(const numerical::Matrix<3, 3, T>& m, const numerical::Matrix<3, 3, T>& m_adj)
{
        return m[0, 0] * m_adj[0, 0] + m[0, 1] * m_adj[1, 0] + m[0, 2] * m_adj[2, 0];
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> sum_with_transpose(const numerical::Matrix<3, 3, T>& m)
{
        numerical::Matrix<3, 3, T> res;
        for (std::size_t i = 0; i < 3; ++i)
        {
                for (std::size_t j = i + 1; j < 3; ++j)
                {
                        const T v = m[i, j] + m[j, i];
                        res[i, j] = v;
                        res[j, i] = v;
                }
        }
        for (std::size_t i = 0; i < 3; ++i)
        {
                const T v = m[i, i];
                res[i, i] = v + v;
        }
        return res;
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> negate_and_add_diagonal(const numerical::Matrix<3, 3, T>& m, const T v)
{
        numerical::Matrix<3, 3, T> res;
        for (std::size_t i = 0; i < 3; ++i)
        {
                for (std::size_t j = 0; j < 3; ++j)
                {
                        res[i, j] = -m[i, j];
                }
        }
        for (std::size_t i = 0; i < 3; ++i)
        {
                res[i, i] += v;
        }
        return res;
}
}
