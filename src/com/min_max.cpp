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

#include "min_max.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <algorithm>
#include <random>
#include <vector>

namespace ns
{
namespace
{
template <typename T>
constexpr bool static_test()
{
        static_assert(0 == min_value(std::span<const T>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})));
        static_assert(9 == max_value(std::span<const T>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})));
        static_assert(0 == min_value(std::span<const T>({10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                                                         0,  1,  2,  3,  4,  5,  6,  7,  8,  9})));
        static_assert(19 == max_value(std::span<const T>({10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                                                          0,  1,  2,  3,  4,  5,  6,  7,  8,  9})));
        return true;
}
static_assert(static_test<float>());
static_assert(static_test<double>());

template <typename T>
void check_equal(const T& v1, const T& v2, const char* text)
{
        if (!(v1 == v2))
        {
                error(std::string("Error finding ") + text + ", " + to_string(v1) + " != " + to_string(v2));
        }
}

template <typename T, typename Engine>
void test_min_max(Engine& engine)
{
        const std::vector<T> data = [&]
        {
                std::vector<T> t(std::uniform_int_distribution<std::size_t>(1, 100)(engine));
                for (T& v : t)
                {
                        v = std::uniform_real_distribution<T>(-10, 10)(engine);
                }
                return t;
        }();

        {
                const T v1 = min_value(std::span<const T>(data));
                const T v2 = *std::min_element(data.cbegin(), data.cend());
                check_equal(v1, v2, "minimum");
        }
        {
                const T v1 = max_value(std::span<const T>(data));
                const T v2 = *std::max_element(data.cbegin(), data.cend());
                check_equal(v1, v2, "maximum");
        }
}

template <typename T, typename Engine>
void test_min_max_performance(Engine& engine)
{
        const std::string type_str = std::string("<") + type_name<T>() + ">";

        const std::vector<T> data = [&]
        {
                std::vector<T> t(200'000'000);
                for (T& v : t)
                {
                        v = std::uniform_real_distribution<T>(-10, 10)(engine);
                }
                return t;
        }();

        {
                Clock::time_point t1 = Clock::now();
                const T v1 = min_value(std::span<const T>(data));
                const double d1 = duration_from(t1);
                LOG("       min_value" + type_str + " = " + to_string(d1) + " seconds");

                Clock::time_point t2 = Clock::now();
                const T v2 = *std::min_element(data.cbegin(), data.cend());
                const double d2 = duration_from(t2);
                LOG("std::min_element" + type_str + " = " + to_string(d2) + " seconds");

                check_equal<T>(v1, v2, "minimum");
        }
        {
                Clock::time_point t1 = Clock::now();
                const T v1 = max_value(std::span<const T>(data));
                const double d1 = duration_from(t1);
                LOG("       max_value" + type_str + " = " + to_string(d1) + " seconds");

                Clock::time_point t2 = Clock::now();
                const T v2 = *std::max_element(data.cbegin(), data.cend());
                const double d2 = duration_from(t2);
                LOG("std::max_element" + type_str + " = " + to_string(d2) + " seconds");

                check_equal<T>(v1, v2, "maximum");
        }
}

void test_equal()
{
        LOG("Test minimum and maximum");

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        for (int i = 0; i < 10; ++i)
        {
                test_min_max<float>(engine);
                test_min_max<double>(engine);
        }

        LOG("Test minimum and maximum passed");
}

void test_performance()
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        test_min_max_performance<float>(engine);
        test_min_max_performance<double>(engine);
}

TEST_SMALL("Algorithm minimum and maximum", test_equal)
TEST_PERFORMANCE("Algorithm minimum and maximum", test_performance)
}
}
