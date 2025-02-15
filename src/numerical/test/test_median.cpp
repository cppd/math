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
#include <src/numerical/median.h>
#include <src/test/test.h>

namespace ns::numerical
{
namespace
{
template <typename T>
void compare(const T a, const T b)
{
        if (!(a == b))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test()
{
        compare(median_of_sorted_data<T>({}, {2}), T{2});
        compare(median_of_sorted_data<T>({2}, {}), T{2});
        compare(median_of_sorted_data<T>({}, {2, 3}), T{2.5});
        compare(median_of_sorted_data<T>({}, {1, 2, 3}), T{2});
        compare(median_of_sorted_data<T>({1}, {2}), T{1.5});
        compare(median_of_sorted_data<T>({2}, {1}), T{1.5});
        compare(median_of_sorted_data<T>({1}, {1, 1}), T{1});
        compare(median_of_sorted_data<T>({1}, {1, 2}), T{1});
        compare(median_of_sorted_data<T>({1}, {1, 3}), T{1});
        compare(median_of_sorted_data<T>({1, 2}, {1, 2}), T{1.5});
        compare(median_of_sorted_data<T>({1}, {1, 2, 3}), T{1.5});
        compare(median_of_sorted_data<T>({3, 4}, {1, 2}), T{2.5});
        compare(median_of_sorted_data<T>({1, 2}, {3, 4}), T{2.5});
        compare(median_of_sorted_data<T>({1, 2}, {0, 3}), T{1.5});
        compare(median_of_sorted_data<T>({1, 2}, {0, 3, 4}), T{2});
        compare(median_of_sorted_data<T>({1, 4}, {0, 2, 3}), T{2});
}

void test_median()
{
        LOG("Test median");

        test<float>();
        test<double>();
        test<long double>();

        LOG("Test median passed");
}

TEST_SMALL("Median", test_median)
}
}
