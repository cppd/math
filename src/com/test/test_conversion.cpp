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

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>

namespace ns
{
namespace
{
template <typename T>
void compare(const T a, const T b, const T precision)
{
        const T abs = std::abs(a - b);
        if (!(abs <= precision))
        {
                error("Conversion abs error: " + to_string(a) + " is not equal to " + to_string(b));
        }

        const T rel = abs / std::max(std::abs(a), std::abs(b));
        if (!(rel <= precision))
        {
                error("Conversion rel error: " + to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test(const T precision)
{
        static_assert(T{10} / 150 * T{25.4L} == pixels_to_millimeters<T>(10, 150));
        static_assert(381 == pixels_to_millimeters<T>(150, 10));
        static_assert(381 == size_to_ppi<T>(10, 150));
        static_assert(T{10} / 150 * T{25.4L} == size_to_ppi<T>(150, 10));
        static_assert(radians_to_degrees(2 * PI<T>) == 360);
        static_assert(degrees_to_radians(T{360}) == 2 * PI<T>);
        static_assert(mps_to_kph(T{10}) == 36);
        static_assert(kph_to_mps(T{36}) == 10);

        const auto cmp = [&](const T a, const T b)
        {
                compare(a, b, precision);
        };

        cmp(21, points_to_pixels<T>(10, 150));
        cmp(21, points_to_pixels<T>(150, 10));
        cmp(59, millimeters_to_pixels<T>(10, 150));
        cmp(59, millimeters_to_pixels<T>(150, 10));
        cmp(381, size_to_ppi<T>(10, 150));
        cmp(25.4L / 15, size_to_ppi<T>(150, 10));
        cmp(25.4L / 15, pixels_to_millimeters<T>(10, 150));
        cmp(381, pixels_to_millimeters<T>(150, 10));
        cmp(36, mps_to_kph<T>(10));
        cmp(10, kph_to_mps<T>(36));
}

void test_conversion()
{
        test<float>(1e-6);
        test<double>(1e-15);
        test<long double>(1e-18);
}

TEST_SMALL("Conversion", test_conversion)
}
}
