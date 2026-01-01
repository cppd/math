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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/create.h>
#include <src/com/random/name.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/progress/progress.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

namespace ns
{
namespace
{
constexpr int MAX_NAME_LENGTH = 18;

template <typename T, typename RandomEngine>
void test_random_engine()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int COUNT = 20'000'000;
        constexpr std::string_view NAME = random_engine_name<RandomEngine>();

        auto engine = create_engine<RandomEngine>();
        std::uniform_real_distribution<T> urd(0, 1);

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(urd(engine));
        }
        const long long performance = std::llround(COUNT / duration_from(start_time));

        const int name_length = NAME.size();
        std::ostringstream oss;
        oss << type_name<T>() << ", ";
        oss << NAME << std::string(std::max(0, MAX_NAME_LENGTH - name_length), ' ');
        oss << ": " << to_string_digit_groups(performance) << " o/s";
        LOG(oss.str());
}

template <typename T, typename Counter>
void compare_random_engines(const Counter& counter)
{
        counter();
        test_random_engine<T, std::knuth_b>();
        counter();
        test_random_engine<T, std::minstd_rand>();
        counter();
        test_random_engine<T, std::minstd_rand0>();
        counter();
        test_random_engine<T, std::mt19937>();
        counter();
        test_random_engine<T, std::mt19937_64>();
        counter();
        test_random_engine<T, PCG>();
        counter();
        test_random_engine<T, std::ranlux24>();
        counter();
        test_random_engine<T, std::ranlux24_base>();
        counter();
        test_random_engine<T, std::ranlux48>();
        counter();
        test_random_engine<T, std::ranlux48_base>();
}

void compare(progress::Ratio* const progress)
{
        constexpr int COUNT = 10 * 3;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };
        compare_random_engines<float>(counter);
        compare_random_engines<double>(counter);
        compare_random_engines<long double>(counter);
}

TEST_PERFORMANCE("Random Engines", compare)
}
}
