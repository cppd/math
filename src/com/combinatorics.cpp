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

#include "combinatorics.h"

namespace ns
{
static_assert(binomial<0, 0>() == 1);
static_assert(binomial<1, 0>() == 1);
static_assert(binomial<1, 1>() == 1);
static_assert(binomial<100, 0>() == 1);
static_assert(binomial<100, 100>() == 1);
static_assert(binomial<100, 1>() == 100);
static_assert(binomial<100, 99>() == 100);
static_assert(binomial<20, 10>() == 184756);
static_assert(binomial<30, 20>() == 30045015);
static_assert(binomial<30, 10>() == 30045015);
static_assert(binomial<40, 30>() == 847660528);
static_assert(binomial<40, 10>() == 847660528);
}
