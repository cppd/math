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

#include "../determinant.h"

namespace ns::numerical
{
namespace
{
// clang-format off
template<typename T>
constexpr std::array<Vector<7, T>, 7> VECTORS =
{{
{10,  2,   3,   4,   5,   6,   7},
{ 8, 90,  10,  11,  12,  13,  14},
{15, 16, 170,  18,  19,  20,  21},
{22, 23,  24, 250,  26,  27,  28},
{29, 30,  31,  32, 330,  34,  35},
{36, 37,  38,  39,  40, 410,  42},
{43, 44,  45,  46,  47,  48, 490}
}};
// clang-format on

constexpr long long DETERMINANT = 1'868'201'030'776'500;

static_assert(DETERMINANT == determinant_by_cofactor_expansion(VECTORS<long long>));
static_assert(DETERMINANT == determinant_by_cofactor_expansion(VECTORS<__int128>));
static_assert(DETERMINANT == determinant_by_cofactor_expansion(VECTORS<double>));
static_assert(DETERMINANT == determinant_by_cofactor_expansion(VECTORS<long double>));
static_assert(DETERMINANT == determinant_by_cofactor_expansion(VECTORS<__float128>));

static_assert(
        -28
        == determinant_by_cofactor_expansion(
                VECTORS<int>,
                std::to_array<unsigned char>({2, 4}),
                std::to_array<unsigned char>({3, 5})));
}
}
