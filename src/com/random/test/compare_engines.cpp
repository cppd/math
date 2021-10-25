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

#include "../engine.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>

namespace ns
{
namespace
{
constexpr int MAX_NAME_LENGTH = 18;

template <typename RandomEngine, typename T>
void test_random_engine(const std::string_view& engine_name)
{
        static_assert(std::is_floating_point_v<T>);

        RandomEngine engine = create_engine<RandomEngine>();
        std::uniform_real_distribution<T> urd(0, 1);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < 100'000'000; ++i)
        {
                do_not_optimize(urd(engine));
        }
        const double d = duration_from(start_time);

        const int name_length = engine_name.size();
        std::ostringstream oss;
        oss << engine_name << std::string(std::max(0, MAX_NAME_LENGTH - name_length), ' ');
        oss << " : time = " << std::setw(5) << std::lround(1000 * d) << " ms";
        LOG(oss.str());
}

#define TEST_RANDOM_ENGINE(engine) test_random_engine<engine, T>(#engine)

template <typename T>
void compare_random_engines()
{
        LOG(std::string("Random engines <") + type_name<T>() + ">");

        TEST_RANDOM_ENGINE(std::knuth_b);
        TEST_RANDOM_ENGINE(std::minstd_rand);
        TEST_RANDOM_ENGINE(std::minstd_rand0);
        TEST_RANDOM_ENGINE(std::mt19937);
        TEST_RANDOM_ENGINE(std::mt19937_64);
        TEST_RANDOM_ENGINE(std::ranlux24);
        TEST_RANDOM_ENGINE(std::ranlux24_base);
        TEST_RANDOM_ENGINE(std::ranlux48);
        TEST_RANDOM_ENGINE(std::ranlux48_base);
}

void compare()
{
        compare_random_engines<float>();
        compare_random_engines<double>();
        compare_random_engines<long double>();
}

TEST_PERFORMANCE("Compare random engines", compare)
}
}
