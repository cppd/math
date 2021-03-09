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

#include "radical_inverse.h"

#include "type/limit.h"

namespace ns
{
static_assert(321.0f / 1000 == radical_inverse<10, float>(123));
static_assert(321.0 / 1000 == radical_inverse<10, double>(123));

static_assert(static_cast<float>(0b101) / 0b10000 == radical_inverse<2, float>(0b1010));
static_assert(static_cast<double>(0b101) / 0b10000 == radical_inverse<2, double>(0b1010));

static_assert(static_cast<float>(0101) / 010000 == radical_inverse<8, float>(01010));
static_assert(static_cast<double>(0101) / 010000 == radical_inverse<8, double>(01010));

static_assert(static_cast<float>(0x321) / 0x1000 == radical_inverse<16, float>(0x123));
static_assert(static_cast<double>(0x321) / 0x1000 == radical_inverse<16, double>(0x123));

static_assert(1 > radical_inverse<2, float>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<3, float>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<4, float>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<5, float>(limits<unsigned long long>::max() - 1));

static_assert(1 > radical_inverse<2, double>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<3, double>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<4, double>(limits<unsigned long long>::max() - 1));
static_assert(1 > radical_inverse<5, double>(limits<unsigned long long>::max() - 1));
}
