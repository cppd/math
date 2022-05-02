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

#include "conversion.h"

#include "error.h"
#include "print.h"

#include <src/test/test.h>

#include <algorithm>
#include <cmath>

namespace ns
{
namespace
{
static_assert(381 == size_to_ppi(10, 150));
static_assert(25.4 / 15 == size_to_ppi(150, 10));
static_assert(25.4 / 15 == pixels_to_millimeters(10, 150));
static_assert(381 == pixels_to_millimeters(150, 10));

void compare(const double a, const double b)
{
        const double abs = std::abs(a - b);
        if (!(abs < 1e-10))
        {
                error("Conversion error: " + to_string(a) + " is not equal to " + to_string(b));
        }

        const double rel = abs / std::max(std::abs(a), std::abs(b));
        if (!(rel < 1e-10))
        {
                error("Conversion error: " + to_string(a) + " is not equal to " + to_string(b));
        }
}

void test()
{
        compare(21, points_to_pixels(10, 150));
        compare(21, points_to_pixels(150, 10));
        compare(59, millimeters_to_pixels(10, 150));
        compare(59, millimeters_to_pixels(150, 10));
        compare(381, size_to_ppi(10, 150));
        compare(25.4 / 15, size_to_ppi(150, 10));
        compare(25.4 / 15, pixels_to_millimeters(10, 150));
        compare(381, pixels_to_millimeters(150, 10));
}

TEST_SMALL("Conversion", test)
}
}
