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

#include "compare.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/statistics/moving_variance.h>
#include <src/statistics/utils.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>

namespace ns::statistics::test
{
namespace
{
template <typename T>
void check_empty(const MovingVariance<T>& variance)
{
        if (variance.has_mean())
        {
                error("Variance has mean");
        }

        if (variance.has_variance())
        {
                error("Variance has variance");
        }

        if (!(variance.size() == 0))
        {
                error("Variance is not empty");
        }
}

template <typename T>
void check_one(const MovingVariance<T>& variance)
{
        if (!variance.has_mean())
        {
                error("Variance has no mean");
        }

        if (!variance.has_variance())
        {
                error("Variance has no variance");
        }

        if (!(variance.size() == 1))
        {
                error("Variance data size " + to_string(variance.size()) + " is not equal to 1");
        }
}

template <typename T>
void check_two(const MovingVariance<T>& variance)
{
        if (!variance.has_mean())
        {
                error("Variance has no mean");
        }

        if (!variance.has_variance())
        {
                error("Variance has no variance");
        }

        if (!(variance.size() >= 2))
        {
                error("Variance data size " + to_string(variance.size()) + " is not greater than or equal to 2");
        }
}

template <typename T>
void test(const T& precision)
{
        const auto cmp = [&](const T& a, const T& b)
        {
                compare(a, b, precision);
        };

        static constexpr std::size_t WINDOW_SIZE = 3;

        MovingVariance<T> variance(WINDOW_SIZE);

        check_empty(variance);

        variance.push(T{1});

        check_one(variance);

        cmp(T{1}, variance.mean());
        cmp(T{0}, variance.variance());

        struct Data final
        {
                T value;
                T mean;
                T variance;
        };

        constexpr std::array DATA = std::to_array<Data>({
                { T{2},  T{3} / T{2},   T{1} / T{4}},
                {T{-2},  T{1} / T{3},  T{26} / T{9}},
                {T{10}, T{10} / T{3}, T{224} / T{9}},
                { T{3}, T{11} / T{3}, T{218} / T{9}},
                {T{-8},  T{5} / T{3}, T{494} / T{9}},
                { T{1}, T{-4} / T{3}, T{206} / T{9}},
                { T{9},  T{2} / T{3}, T{434} / T{9}}
        });

        for (const Data& d : DATA)
        {
                variance.push(d.value);

                check_two(variance);

                cmp(d.mean, variance.mean());
                cmp(d.variance, variance.variance());
                cmp(utils::sqrt(d.variance), variance.standard_deviation());
        }

        if (!(variance.size() == 3))
        {
                error("Variance data size " + to_string(variance.size()) + " is not equal to "
                      + to_string(WINDOW_SIZE));
        }
}

void test_variance()
{
        LOG("Test moving variance");

        test<float>(1e-5);
        test<double>(1e-13);
        test<long double>(1e-17);

        test(numerical::Vector<3, float>(1e-5));
        test(numerical::Vector<3, double>(1e-13));
        test(numerical::Vector<3, long double>(1e-17));

        LOG("Test moving variance passed");
}

TEST_SMALL("Moving Variance", test_variance)
}
}
