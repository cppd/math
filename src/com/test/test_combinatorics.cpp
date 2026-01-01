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

#include <src/com/combinatorics.h>

#include <array>

namespace ns
{
static_assert(BINOMIAL<0, 0> == 1);
static_assert(BINOMIAL<1, 0> == 1);
static_assert(BINOMIAL<1, 1> == 1);
static_assert(BINOMIAL<100, 0> == 1);
static_assert(BINOMIAL<100, 100> == 1);
static_assert(BINOMIAL<100, 1> == 100);
static_assert(BINOMIAL<100, 99> == 100);
static_assert(BINOMIAL<20, 10> == 184756);
static_assert(BINOMIAL<30, 20> == 30045015);
static_assert(BINOMIAL<30, 10> == 30045015);
static_assert(BINOMIAL<40, 30> == 847660528);
static_assert(BINOMIAL<40, 10> == 847660528);

// clang-format off
static_assert(
        COMBINATIONS<4, 1> == std::to_array<std::array<unsigned char, 1>>
        ({
                 {0}, {1}, {2}, {3}
        }));
static_assert(
        COMBINATIONS<4, 2> == std::to_array<std::array<unsigned char, 2>>
        ({
                {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}
        }));
static_assert(
        COMBINATIONS<4, 3> == std::to_array<std::array<unsigned char, 3>>
        ({
                 {0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}
        }));
static_assert(
        COMBINATIONS<4, 4> == std::to_array<std::array<unsigned char, 4>>
        ({
                 {0, 1, 2, 3}
        }));
// clang-format on

static_assert(FACTORIAL<0> == 1);
static_assert(FACTORIAL<1> == 1);
static_assert(FACTORIAL<2> == 2);
static_assert(FACTORIAL<3> == 6);
static_assert(FACTORIAL<4> == 24);
static_assert(FACTORIAL<5> == 120);
static_assert(FACTORIAL<20> == 2432902008176640000);
}
