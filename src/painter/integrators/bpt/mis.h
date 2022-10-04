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

#include "vertex.h"

#include <src/com/error.h>

#include <vector>

namespace ns::painter::integrators::bpt
{
template <std::size_t N, typename T, typename Color>
[[nodiscard]] T mis_weight(
        const std::vector<Vertex<N, T, Color>>& /*light_path*/,
        const std::vector<Vertex<N, T, Color>>& /*camera_path*/,
        const int s,
        const int t)
{
        ASSERT(t >= 2);
        ASSERT(s >= 0);

        if (s + t == 2)
        {
                return 1;
        }

        return 1;
}
}