/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../variance.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <array>
#include <cmath>

namespace ns::numerical
{
namespace
{
template <typename T>
void compare(const T a, const T b, const T precision)
{
        if (std::abs(a - b) < precision)
        {
                return;
        }
        error(to_string(a) + " is not equal to " + to_string(b));
}

template <typename T>
void test(const T precision)
{
        const auto cmp = [&](const T a, const T b)
        {
                compare(a, b, precision);
        };

        MovingVariance<T> variance(3);

        if (!variance.empty())
        {
                error("Variance is not empty");
        }

        variance.push(1);
        if (variance.empty())
        {
                error("Variance is empty");
        }
        cmp(1, variance.mean());
        cmp(0, variance.variance_n());

        struct Data final
        {
                T value;
                T mean;
                T variance;
                T variance_n;
        };

        constexpr std::array DATA = std::to_array<Data>({
                { 2,  T{3} / 2,   T{1} / 2,   T{1} / 4},
                {-2,  T{1} / 3,  T{13} / 3,  T{26} / 9},
                {10, T{10} / 3, T{112} / 3, T{224} / 9},
                { 3, T{11} / 3, T{109} / 3, T{218} / 9},
                {-8,  T{5} / 3, T{247} / 3, T{494} / 9},
                { 1, T{-4} / 3, T{103} / 3, T{206} / 9},
                { 9,  T{2} / 3, T{217} / 3, T{434} / 9}
        });

        for (const Data& d : DATA)
        {
                variance.push(d.value);
                if (variance.empty())
                {
                        error("Variance is empty");
                }
                cmp(d.mean, variance.mean());
                cmp(d.variance, variance.variance());
                cmp(d.variance_n, variance.variance_n());
        }
}

void test_variance()
{
        LOG("Test variance");
        test<float>(1e-5);
        test<double>(1e-13);
        test<long double>(1e-17);
        LOG("Test variance passed");
}

TEST_SMALL("Variance", test_variance)
}
}
