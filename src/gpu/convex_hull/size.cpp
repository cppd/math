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

#include "size.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/com/print.h>

#include <algorithm>
#include <bit>
#include <cstdint>

namespace ns::gpu::convex_hull
{
int points_buffer_size(const int height)
{
        // 2 lines + 1 point, GLSL ivec2
        return (2 * height + 1) * (2 * sizeof(std::int32_t));
}

int group_size_prepare(
        const int width,
        const unsigned max_group_size_x,
        const unsigned max_group_invocations,
        const unsigned max_shared_memory_size)
{
        constexpr unsigned SHARED_SIZE_PER_THREAD = 2 * sizeof(std::int32_t); // GLSL ivec2

        const unsigned max_group_size_limit = std::min(max_group_size_x, max_group_invocations);
        const unsigned max_group_size_memory = max_shared_memory_size / SHARED_SIZE_PER_THREAD;

        const unsigned max_group_size = std::bit_floor(std::min(max_group_size_limit, max_group_size_memory));

        // one thread for 2 or more pixels, power of 2
        const unsigned pref_thread_count = (width > 1) ? (std::bit_floor(static_cast<unsigned>(width) - 1)) : 1;

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int group_size_merge(
        const int height,
        const unsigned max_group_size_x,
        const unsigned max_group_invocations,
        const unsigned max_shared_memory_size)
{
        static_assert(sizeof(float) == 4);

        constexpr unsigned SHARED_SIZE_PER_ITEM = sizeof(float); // GLSL float

        if (max_shared_memory_size < height * SHARED_SIZE_PER_ITEM)
        {
                error("Shared memory problem: needs " + to_string(height * SHARED_SIZE_PER_ITEM) + ", exists "
                      + to_string(max_shared_memory_size));
        }

        const int max_group_size = std::min(max_group_size_x, max_group_invocations);

        // one thread for a group for up to 4 items.
        const int pref_thread_count = group_count(height, 4);

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int iteration_count_merge(const unsigned size)
{
        // Starts with groups of 4 items, the right half of the group starts with index 2.
        // Increase the index by 2 times at each iteration.
        // This index must be strictly less than size.
        return (size > 2) ? (std::bit_width(size - 1) - 1) : 0;
}
}
