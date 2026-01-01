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

#include "hyperplane.h"

#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
Hyperplane<N - 1, T> clip_plane_equation_to_clip_plane(const numerical::Vector<N, T>& clip_plane_equation)
{
        // from n * x + d with normal directed inward
        // to n * x - d with normal directed outward
        const T d = clip_plane_equation[N - 1];
        const numerical::Vector<N - 1, T> n = [&]
        {
                numerical::Vector<N - 1, T> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = -clip_plane_equation[i];
                }
                return res;
        }();
        return {n, d};
}
}
