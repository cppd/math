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

#include <src/com/global_index.h>

#include <array>
#include <cstddef>

namespace ns
{
// compute

static_assert(GlobalIndex<1, long long>(std::array<unsigned, 1>{100}).compute(std::array<unsigned char, 1>{10}) == 10);

static_assert(GlobalIndex<2, int>(std::array<int, 2>{1, 200}).compute(std::array<unsigned char, 2>{0, 100}) == 100);

static_assert(
        GlobalIndex<2, int>(std::array<int, 2>{10000, 20000}).compute(std::array<unsigned char, 2>{200, 100})
        == 1000200);

static_assert(
        GlobalIndex<5, __int128>(std::array<unsigned char, 5>{100, 100, 100, 100, 100})
                .compute(std::array<int, 5>{1, 1, 1, 1, 1})
        == 101010101);

static_assert(
        GlobalIndex<5, long long>(std::array<std::size_t, 5>{123, 456, 789, 987, 654})
                .compute(std::array<int, 5>{12, 34, 56, 78, 98})
        == 4283912376450);

static_assert(
        GlobalIndex<5, __int128>(std::array<long long, 5>{123456, 789876, 543212, 345678, 987654})
                .compute(std::array<int, 5>{12345, 67898, 76543, 21234, 56789})
        == __int128{1039864870365} * __int128{1000000000000000} + 704301544246713);

// count

static_assert(GlobalIndex<1, long long>(std::array<unsigned, 1>{100}).count() == 100);

static_assert(GlobalIndex<2, int>(std::array<int, 2>{1, 200}).count() == 200);

static_assert(GlobalIndex<2, int>(std::array<int, 2>{10000, 20000}).count() == 200000000);

static_assert(GlobalIndex<5, __int128>(std::array<unsigned char, 5>{100, 100, 100, 100, 100}).count() == 10000000000);

static_assert(GlobalIndex<5, long long>(std::array<std::size_t, 5>{123, 456, 789, 987, 654}).count() == 28565501849136);

static_assert(
        GlobalIndex<5, __int128>(std::array<long long, 5>{123456, 789876, 543212, 345678, 987654}).count()
        == __int128{18084938769185} * __int128{1000000000000000} + 969371161636864);

// stride

static_assert(
        GlobalIndex<5, unsigned __int128>(std::array<long long, 5>{11111, 22222, 33333, 44444, 55555}).stride(0) == 1);

static_assert(GlobalIndex<5, __int128>(std::array<long long, 5>{11111, 22222, 33333, 44444, 55555}).stride(1) == 11111);

static_assert(
        GlobalIndex<5, unsigned __int128>(std::array<long long, 5>{11111, 22222, 33333, 44444, 55555}).stride(2)
        == __int128{11111} * 22222);

static_assert(
        GlobalIndex<5, __int128>(std::array<long long, 5>{11111, 22222, 33333, 44444, 55555}).stride(3)
        == __int128{11111} * 22222 * 33333);

static_assert(
        GlobalIndex<5, unsigned __int128>(std::array<long long, 5>{11111, 22222, 33333, 44444, 55555}).stride(4)
        == __int128{11111} * 22222 * 33333 * 44444);
}
