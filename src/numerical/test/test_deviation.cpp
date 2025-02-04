/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/numerical/deviation.h>
#include <src/test/test.h>

#include <cmath>
#include <vector>

namespace ns::numerical
{
namespace
{
template <typename T>
        requires (std::is_floating_point_v<T>)
void compare(const T a, const T b, const T precision)
{
        if (!(std::abs(a - b) <= precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test(const T& precision)
{
        const std::vector<T> data{-2, 3, 7, -15, -6, 0, 1, 3, 19};

        const numerical::MedianAbsoluteDeviation<T> mad = median_absolute_deviation(data);
        compare(mad.median, T{1}, precision);
        compare(mad.deviation, T{3}, precision);

        const T sd = numerical::standard_deviation(mad);
        compare(sd, T{4.44780665551680558147L}, precision);
}

void test_deviation()
{
        LOG("Test deviation");

        test<float>(0);
        test<double>(0);
        test<long double>(0);

        LOG("Test deviation passed");
}

TEST_SMALL("Deviation", test_deviation)
}
}
