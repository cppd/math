/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/numerical/vec.h>

namespace ns::gpu
{
constexpr int group_count(const int size, const int group_size)
{
        return (size + group_size - 1) / group_size;
}

constexpr Vector2i group_count(const int x, const int y, const Vector2i group_size)
{
        Vector2i res;
        res[0] = group_count(x, group_size[0]);
        res[1] = group_count(y, group_size[1]);
        return res;
}

constexpr Vector3i group_count(const int x, const int y, const int z, const Vector3i group_size)
{
        Vector3i res;
        res[0] = group_count(x, group_size[0]);
        res[1] = group_count(y, group_size[1]);
        res[2] = group_count(z, group_size[2]);
        return res;
}
}
