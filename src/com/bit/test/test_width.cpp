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

#include <src/com/bit/width.h>

namespace ns
{
static_assert(bit_width(0u) == 0);
static_assert(bit_width(1u) == 1);
static_assert(bit_width(2u) == 2);
static_assert(bit_width(3u) == 2);
static_assert(bit_width(4u) == 3);

static_assert(bit_width(0b1'0000u) == 5);
static_assert(bit_width(0b1'0001u) == 5);
static_assert(bit_width(0b1'0101u) == 5);
static_assert(bit_width(0b1'1111u) == 5);

static_assert(bit_width(static_cast<unsigned __int128>(0b1'0000)) == 5);
static_assert(bit_width(static_cast<unsigned __int128>(0b1'0001)) == 5);
static_assert(bit_width(static_cast<unsigned __int128>(0b1'0101)) == 5);
static_assert(bit_width(static_cast<unsigned __int128>(0b1'1111)) == 5);
}
