/*
Copyright (C) 2017-2022 Topological Manifold

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

namespace ns::gpu
{
namespace std140
{
template <typename T>
using Matrix3 = Matrix<3, 4, T>;

using Matrix3f = Matrix<3, 4, float>;
using Matrix3d = Matrix<3, 4, double>;
}

template <typename Dst, typename Src>
        requires(!std::is_same_v<Dst, Src>)
[[nodiscard]] Matrix<4, 4, Dst> to_std140(const Matrix<4, 4, Src>& m)
{
        Matrix<4, 4, Dst> res;
        for (int r = 0; r < 4; ++r)
        {
                for (int c = 0; c < 4; ++c)
                {
                        res(c, r) = m(r, c);
                }
        }
        return res;
}

template <typename Dst, typename Src>
        requires(std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_std140(const Matrix<4, 4, Src>& m)
{
        return m;
}

template <typename Dst, typename Src>
        requires(std::is_same_v<Dst, Src>)
[[nodiscard]] decltype(auto) to_std140(Matrix<4, 4, Src>&& m)
{
        return std::move(m);
}

template <typename Dst, typename Src>
[[nodiscard]] std140::Matrix3<Dst> to_std140(const Matrix<3, 3, Src>& m)
{
        std140::Matrix3<Dst> res;
        for (int r = 0; r < 3; ++r)
        {
                for (int c = 0; c < 3; ++c)
                {
                        res(c, r) = m(r, c);
                }
        }
        return res;
}
}
