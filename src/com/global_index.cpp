/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "global_index.h"

static_assert(GlobalIndex<1, long long>(std::array<unsigned, 1>{{100}}).index(std::array<unsigned char, 1>{{10}}) == 10);

static_assert(GlobalIndex<2, int>(std::array<int, 2>{{1, 200}}).index(std::array<unsigned char, 2>{{0, 100}}) == 100);

static_assert(GlobalIndex<2, int>(std::array<int, 2>{{10000, 20000}}).index(std::array<unsigned char, 2>{{200, 100}}) == 1000200);

static_assert(GlobalIndex<5, __int128>(std::array<unsigned char, 5>{{100, 100, 100, 100, 100}})
                      .index(std::array<signed char, 5>{{1, 1, 1, 1, 1}}) == 101010101);

static_assert(GlobalIndex<5, long long>(std::array<std::size_t, 5>{{123, 456, 789, 987, 654}})
                      .index(std::array<signed char, 5>{{12, 34, 56, 78, 98}}) == 4283912376450);

static_assert(GlobalIndex<5, __int128>(std::array<long long, 5>{{123456, 789876, 543212, 345678, 987654}})
                      .index(std::array<int, 5>{{12345, 67898, 76543, 21234, 56789}}) ==
              __int128(1039864870365) * __int128(1000000000000000) + 704301544246713);
