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

#include "size.h"

#include "../com/groups.h"

#include <src/com/bits.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>

namespace gpu::convex_hull
{
int points_buffer_size(int height)
{
        // 2 линии точек + 1 точка, тип ivec2
        return (2 * height + 1) * (2 * sizeof(int32_t));
}

int group_size_prepare(
        int width,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size)
{
        unsigned shared_size_per_thread = 2 * sizeof(int32_t); // GLSL ivec2

        int max_group_size_limit = std::min(max_group_size_x, max_group_invocations);
        int max_group_size_memory = max_shared_memory_size / shared_size_per_thread;

        // максимально возможная степень 2
        int max_group_size = 1 << log_2(std::min(max_group_size_limit, max_group_size_memory));

        // один поток обрабатывает 2 и более пикселей, при этом число потоков должно быть степенью 2.
        int pref_thread_count = (width > 1) ? (1 << log_2(width - 1)) : 1;

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int group_size_merge(
        int height,
        unsigned max_group_size_x,
        unsigned max_group_invocations,
        unsigned max_shared_memory_size)
{
        static_assert(sizeof(float) == 4);

        unsigned shared_size_per_item = sizeof(float); // GLSL float

        if (max_shared_memory_size < height * shared_size_per_item)
        {
                error("Shared memory problem: needs " + to_string(height * shared_size_per_item) + ", exists "
                      + to_string(max_shared_memory_size));
        }

        int max_group_size = std::min(max_group_size_x, max_group_invocations);

        // Один поток первоначально обрабатывает группы до 4 элементов.
        int pref_thread_count = group_count(height, 4);

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int iteration_count_merge(int size)
{
        // Расчёт начинается с 4 элементов, правый средний индекс (начало второй половины) равен 2.
        // На каждой итерации индекс увеличивается в 2 раза.
        // Этот индекс должен быть строго меньше заданного числа size.
        // Поэтому число итераций равно максимальной степени 2, в которой число 2 строго меньше заданного числа size.
        return (size > 2) ? log_2(size - 1) : 0;
}
}
