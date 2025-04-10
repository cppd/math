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

#include "compare.h"

#include <src/com/log.h>
#include <src/statistics/median.h>
#include <src/test/test.h>

#include <algorithm>
#include <vector>

namespace ns::statistics::test
{
namespace
{
template <typename T>
void test()
{
        {
                std::vector<T> data{5, 2, 4, 1, 3};
                std::vector<T> v = data;
                compare(median(&v), T{3});

                std::ranges::sort(data);
                std::ranges::sort(v);
                compare(v, data);
        }
        {
                std::vector<T> data{5, 2, 4, 3};
                std::vector<T> v = data;
                compare(median(&v), T{3.5});

                std::ranges::sort(data);
                std::ranges::sort(v);
                compare(v, data);
        }
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
