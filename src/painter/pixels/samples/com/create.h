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

#include "info.h"

#include <src/com/error.h>

#include <cstddef>

namespace ns::painter::pixels::samples::com
{
template <typename Samples, typename Copy, typename Sum>
void create_with_sum(const std::size_t count, const Copy copy, const Sum sum)
{
        static constexpr std::size_t COUNT = Info<Samples>::COUNT;
        static_assert(COUNT % 2 == 0);

        ASSERT(count > COUNT);

        std::size_t sample_i = 0;

        for (std::size_t i = 0; i < COUNT / 2; ++i, ++sample_i)
        {
                copy(i, sample_i);
        }

        const std::size_t sum_end = count - COUNT / 2;
        for (; sample_i < sum_end; ++sample_i)
        {
                sum(sample_i);
        }

        for (std::size_t i = COUNT / 2; i < COUNT; ++i, ++sample_i)
        {
                copy(i, sample_i);
        }

        ASSERT(sample_i == count);
}
}
