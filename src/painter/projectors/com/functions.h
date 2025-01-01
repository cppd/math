/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::painter::projectors::com
{
template <std::size_t N, typename T>
void check_orthogonality(
        const numerical::Vector<N, T>& camera_dir,
        const std::array<numerical::Vector<N, T>, N - 1>& screen_axes)
{
        constexpr T LIMIT_COS = Limits<T>::epsilon() * 100;

        for (std::size_t i = 0; i < N - 1; ++i)
        {
                if (!(std::abs(dot(screen_axes[i], camera_dir)) <= LIMIT_COS))
                {
                        error("The screen axis " + to_string(i) + " is not orthogonal to the camera direction");
                }

                for (std::size_t j = i + 1; j < N - 1; ++j)
                {
                        if (!(std::abs(dot(screen_axes[i], screen_axes[j])) <= LIMIT_COS))
                        {
                                error("The screen axis " + to_string(i) + " is not orthogonal to the screen axes "
                                      + to_string(j));
                        }
                }
        }
}

template <std::size_t N, typename T>
std::array<numerical::Vector<N, T>, N - 1> normalize_axes(const std::array<numerical::Vector<N, T>, N - 1>& axes)
{
        std::array<numerical::Vector<N, T>, N - 1> res;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res[i] = axes[i].normalized();
        }
        return res;
}

template <typename T, std::size_t N>
numerical::Vector<N, T> screen_org(const std::array<int, N>& screen_size)
{
        numerical::Vector<N, T> org;
        for (std::size_t i = 0; i < N; ++i)
        {
                if (screen_size[i] < 1)
                {
                        error("Screen size " + to_string(i) + " is not positive (" + to_string(screen_size[i]) + ")");
                }
                org[i] = -screen_size[i] * (static_cast<T>(1) / 2);
        }
        return org;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> screen_dir(
        const std::array<numerical::Vector<N, T>, N - 1>& screen_axes,
        const numerical::Vector<N - 1, T>& screen_point)
{
        numerical::Vector<N, T> dir = screen_axes[0] * screen_point[0];
        for (std::size_t i = 1; i < N - 1; ++i)
        {
                dir.multiply_add(screen_axes[i], screen_point[i]);
        }
        return dir;
}
}
