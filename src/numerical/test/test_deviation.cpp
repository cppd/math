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
#include <src/com/random/pcg.h>
#include <src/numerical/deviation.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

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
void test_constant()
{
        {
                const std::vector<T> data{-2, 3, 7, -15, -6, 0, 1, 3, 19};

                const numerical::MedianAbsoluteDeviation<T> mad = median_absolute_deviation(data);
                compare(mad.median, T{1});
                compare(mad.deviation, T{3});

                const T sd = numerical::standard_deviation(mad);
                compare(sd, T{4.44780665551680558147L});
        }

        {
                const std::vector<T> data{-2, 3, 2, 7, -15, -6, 0, 1, 3, 19};

                const numerical::MedianAbsoluteDeviation<T> mad = median_absolute_deviation(data);
                compare(mad.median, T{1.5});
                compare(mad.deviation, T{2.5});

                const T sd = numerical::standard_deviation(mad);
                compare(sd, T{3.70650554626400465137L});
        }
}

template <typename T>
void test_random()
{
        constexpr std::size_t COUNT = 10'000;
        constexpr std::size_t ERROR_COUNT = 10;
        constexpr T MEAN = -1;
        constexpr T STD_DEV = 10;

        const std::vector<T> data = []
        {
                PCG engine;
                std::normal_distribution<T> nd(MEAN, STD_DEV);
                std::vector<T> res;
                res.reserve(COUNT + ERROR_COUNT);
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                        res.push_back(nd(engine));
                }
                for (std::size_t i = 1; i <= ERROR_COUNT; ++i)
                {
                        res.push_back(MEAN + T{10'000} * i * STD_DEV);
                }
                return res;
        }();

        const numerical::MedianAbsoluteDeviation<T> mad = median_absolute_deviation(data);
        const T sd = numerical::standard_deviation(mad);

        if (!(mad.median > -2 && mad.median < 0))
        {
                error("Median " + to_string(mad.median) + " is out of range");
        }

        if (!(mad.deviation > 6 && mad.deviation < 7.5))
        {
                error("Deviation " + to_string(mad.deviation) + " is out of range");
        }

        if (!(sd > 9 && mad.deviation < 11))
        {
                error("Standard deviation " + to_string(sd) + " is out of range");
        }
}

template <typename T>
void test()
{
        test_constant<T>();
        test_random<T>();
}

void test_deviation()
{
        LOG("Test deviation");

        test<float>();
        test<double>();
        test<long double>();

        LOG("Test deviation passed");
}

TEST_SMALL("Deviation", test_deviation)
}
}
