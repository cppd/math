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

#include "determinant.h"

namespace ns::numerical
{
namespace determinant_implementation
{
// clang-format off
static_assert
(
        1'868'201'030'776'500
        ==
        determinant<7, 7, __int128, 7>
        (
        {{
        {10,  2,   3,   4,   5,   6,   7},
        { 8, 90,  10,  11,  12,  13,  14},
        {15, 16, 170,  18,  19,  20,  21},
        {22, 23,  24, 250,  26,  27,  28},
        {29, 30,  31,  32, 330,  34,  35},
        {36, 37,  38,  39,  40, 410,  42},
        {43, 44,  45,  46,  47,  48, 490}
        }},
        sequence_uchar_array<7>,
        sequence_uchar_array<7>
        )
);
// clang-format on
}
}
