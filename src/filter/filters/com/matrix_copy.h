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
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::com
{
template <typename T, std::size_t M>
void copy_position_velocity_p(
        const numerical::Matrix<4, 4, T>& position_velocity_p,
        numerical::Matrix<M, M, T>& position_velocity_acceleration_p)
{
        static_assert(M >= 6);

        static constexpr std::size_t N = 2;

        const numerical::Matrix<4, 4, T>& p = position_velocity_p;
        numerical::Matrix<M, M, T>& res = position_velocity_acceleration_p;

        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t i = 0; i < 2; ++i)
                {
                        const numerical::Vector<4, T>& row_p = p.row(2 * r + i);
                        numerical::Vector<M, T>& row_res = res.row(3 * r + i);

                        for (std::size_t c = 0; c < N; ++c)
                        {
                                for (std::size_t j = 0; j < 2; ++j)
                                {
                                        row_res[3 * c + j] = row_p[2 * c + j];
                                }
                        }
                }
        }
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<2 * (N / 3), 2 * (N / 3), T> copy_position_velocity_from_p(
        const numerical::Matrix<N, N, T>& p)
{
        static_assert(N >= 3);
        static_assert(N % 3 == 0);

        static constexpr std::size_t S = N / 3;

        numerical::Matrix<2 * S, 2 * S, T> res;
        for (std::size_t r = 0; r < S; ++r)
        {
                for (std::size_t i = 0; i < 2; ++i)
                {
                        const numerical::Vector<N, T>& row_p = p.row(3 * r + i);
                        numerical::Vector<2 * S, T>& row_res = res.row(2 * r + i);

                        for (std::size_t c = 0; c < S; ++c)
                        {
                                for (std::size_t j = 0; j < 2; ++j)
                                {
                                        row_res[2 * c + j] = row_p[3 * c + j];
                                }
                        }
                }
        }
        return res;
}
}
