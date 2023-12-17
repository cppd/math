/*
Copyright (C) 2017-2023 Topological Manifold

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

namespace ns::gpu::convex_hull
{
int points_buffer_size(int height);

int group_size_prepare(
        int width,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size);

int group_size_merge(
        int height,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size);

int iteration_count_merge(unsigned size);
}
