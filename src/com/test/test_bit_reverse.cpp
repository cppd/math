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

#include "../bit_reverse.h"

namespace ns
{
static_assert(bit_reverse(4, 0b1011) == 0b1101);
static_assert(bit_reverse(31, 0b1011001110001111000011111000001) == 0b1000001111100001111000111001101);
static_assert(bit_reverse_8(0b10110011) == 0b11001101);
static_assert(bit_reverse(8, 0b10110011) == 0b11001101);
static_assert(bit_reverse_16(0b1011001110001111) == 0b1111000111001101);
static_assert(bit_reverse(16, 0b1011001110001111) == 0b1111000111001101);
static_assert(bit_reverse_32(0b10110011100011110000111110000011) == 0b11000001111100001111000111001101);
static_assert(bit_reverse(32, 0b10110011100011110000111110000011) == 0b11000001111100001111000111001101);
static_assert(
        bit_reverse_64(0b1011001110001111000011111000001111110000001111111000000011111111)
        == 0b1111111100000001111111000000111111000001111100001111000111001101);
static_assert(
        bit_reverse(64, 0b1011001110001111000011111000001111110000001111111000000011111111)
        == 0b1111111100000001111111000000111111000001111100001111000111001101);
}