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

#include "../create.h"
#include "../name.h"
#include "../pcg.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns
{
namespace
{
std::string to_string(const std::unordered_map<int, int>& map)
{
        std::vector<std::array<int, 2>> data;
        data.reserve(map.size());
        for (const auto& [key, value] : map)
        {
                data.push_back({key, value});
        }

        std::sort(
                data.begin(), data.end(),
                [](const auto& v1, const auto& v2)
                {
                        return v1[0] < v2[0];
                });

        std::string res;
        for (const auto [key, value] : data)
        {
                if (!res.empty())
                {
                        res += "\n";
                }
                res += std::to_string(key);
                res += ": ";
                res += std::to_string(value);
        }
        return res;
}

template <typename T>
std::unordered_map<int, int> test_distribution(T&& engine)
{
        constexpr int KEY_COUNT = 10;
        constexpr int VALUE_COUNT = 10'000;
        constexpr int MIN = VALUE_COUNT - VALUE_COUNT / 25;
        constexpr int MAX = VALUE_COUNT + VALUE_COUNT / 25;

        std::unordered_map<int, int> map;
        std::uniform_int_distribution<int> uid(0, KEY_COUNT - 1);

        for (int i = 0; i < VALUE_COUNT * KEY_COUNT; ++i)
        {
                ++map[uid(engine)];
        }

        for (const auto& v : map)
        {
                if (!(v.second >= MIN && v.second <= MAX))
                {
                        error(std::string(random_engine_name<T>()) + " distribution error (" + std::to_string(v.first)
                              + ": " + std::to_string(v.second) + ")\n" + to_string(map));
                }
        }

        return map;
}

template <typename T>
std::unordered_set<typename T::result_type> test_values(T&& engine)
{
        constexpr std::size_t COUNT = 100'000;
        constexpr std::size_t MIN = COUNT - COUNT / 10'000;

        std::unordered_set<typename T::result_type> set;

        for (std::size_t i = 0; i < COUNT; ++i)
        {
                set.insert(engine());
        }

        if (!(set.size() >= MIN))
        {
                error(std::string(random_engine_name<T>()) + " unique value count \n" + std::to_string(set.size())
                      + " is too small, generated " + std::to_string(COUNT) + " values");
        }

        return set;
}

template <typename T>
void test_value(T&& engine, const int count, const typename T::result_type expected_value)
{
        if (count < 1)
        {
                error(std::string(random_engine_name<T>()) + " value count " + std::to_string(count)
                      + " must be positive");
        }

        for (int i = 1; i < count; ++i)
        {
                static_cast<void>(engine());
        }

        const auto v = engine();
        if (!(v == expected_value))
        {
                error(std::string(random_engine_name<T>()) + " value error (" + std::to_string(v) + "), expected "
                      + std::to_string(expected_value));
        }
}

template <typename T>
void test_engine()
{
        static_assert(std::uniform_random_bit_generator<PCG>);

        test_distribution(T());
        test_values(T());

        test_distribution(create_engine<T>());
        test_values(create_engine<T>());

        if (!(test_distribution(T()) != test_distribution(T())))
        {
                error(std::string(random_engine_name<T>()) + " random distribution error, results are equal");
        }

        if (!(test_values(T()) != test_values(T())))
        {
                error(std::string(random_engine_name<T>()) + " random value error, results are equal");
        }

        constexpr int V_1 = 1;
        constexpr int V_2 = 2;

        if (!(test_distribution(T(V_1)) == test_distribution(T(V_1))))
        {
                error(std::string(random_engine_name<T>()) + " distribution error, results are not equal");
        }

        if (!(test_values(T(V_1)) == test_values(T(V_1))))
        {
                error(std::string(random_engine_name<T>()) + " value error, results are not equal");
        }

        if (!(test_distribution(T(V_1)) != test_distribution(T(V_2))))
        {
                error(std::string(random_engine_name<T>()) + " distribution error, results are equal");
        }

        if (!(test_values(T(V_1)) != test_values(T(V_2))))
        {
                error(std::string(random_engine_name<T>()) + " value error, results are equal");
        }
}

void test()
{
        LOG("Test PCG");

        test_engine<PCG>();

        test_value(PCG(0), 1000, 1'557'370'411);
        test_value(PCG(1000), 1000, 2'243'789'472);

        LOG("Test PCG passed");
}

TEST_SMALL("PCG", test)
}
}
