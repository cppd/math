/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "test_sphere.h"

#include "../sphere.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/random.h>
#include <src/utility/random/engine.h>

#include <map>

namespace painter
{
namespace
{
template <typename T>
T cos_to_angle(T cosine)
{
        return std::acos(cosine) / PI<T> * 180;
}

template <size_t N, typename T>
T component_sum(const Vector<N, T>& v)
{
        T sum = 0;
        for (unsigned i = 0; i < N; ++i)
        {
                sum += v[i];
        }
        return sum;
}

template <typename Map>
void normalize(Map* map)
{
        using T = typename Map::mapped_type;

        T max = limits<T>::lowest();

        for (const auto& i : *map)
        {
                max = std::max(max, i.second);
        }

        for (auto& i : *map)
        {
                i.second /= max;
        }
}

template <size_t N, typename T, typename RandomEngine>
void test_distribution(int count, T discrepancy_limit)
{
        LOG("Test Distribution...");

        constexpr T DISCRETIZATION = 100;

        RandomEngineWithSeed<RandomEngine> random_engine;

        std::map<T, T, std::greater<T>> buckets;

        std::uniform_real_distribution<T> urd(-1, 1);
        Vector<N, T> normal = random_vector<N, T>(random_engine, urd).normalized();

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> random_vector = random_cosine_weighted_on_hemisphere(random_engine, normal).normalized();

                T cosine;

                cosine = dot(random_vector, normal);
                cosine = std::ceil(cosine * DISCRETIZATION) / DISCRETIZATION;
                cosine = std::min(static_cast<T>(1), cosine);

                // Если 0, то это тоже не полусфера, но 0 может быть из-за ошибок округления
                if (cosine < 0)
                {
                        error("Not hemisphere vector");
                }

                if (auto iter = buckets.find(cosine); iter != buckets.end())
                {
                        ++(iter->second);
                }
                else
                {
                        buckets.emplace(cosine, 1);
                }
        }

        normalize(&buckets);

        for (const auto& [cosine, value] : buckets)
        {
                T discrepancy = std::abs(value - cosine);

                if (discrepancy > discrepancy_limit)
                {
                        LOG("angle = " + to_string(cos_to_angle(cosine), 5) + ", cos = " + to_string(cosine, 5)
                            + ", value = " + to_string(value, 5) + ", d = " + to_string(discrepancy, 5));

                        error("Huge discrepancy");
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
void test_speed(int count)
{
        LOG("Test Speed...");

        RandomEngineWithSeed<RandomEngine> random_engine;

        std::uniform_real_distribution<T> urd(-1, 1);

        std::vector<Vector<N, T>> data;
        data.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                data.push_back(random_vector<N, T>(random_engine, urd).normalized());
        }

        TimePoint start_time = time();

        // Сумма для того, чтобы избежать оптимизаций компилятора из-за неиспользуемого значения функции
        Vector<N, T> sum(0);
        for (const Vector<N, T>& n : data)
        {
                sum += random_cosine_weighted_on_hemisphere(random_engine, n);
        }

        LOG("Time = " + to_string_fixed(duration_from(start_time), 5)
            + " seconds, sum = " + to_string(component_sum(sum)));
}

template <size_t N, typename T, typename RandomEngine>
void test_cosine_hemisphere(int count, T discrepancy_limit)
{
        LOG("Test in " + space_name(N) + ", " + to_string_digit_groups(count) + ", " + type_name<T>());

        test_distribution<N, T, RandomEngine>(count, discrepancy_limit);

        test_speed<N, T, RandomEngine>(count);
}

template <typename T, typename RandomEngine>
void test_cosine_hemisphere(int count, T discrepancy_limit)
{
        test_cosine_hemisphere<3, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<4, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<5, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<6, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<7, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<8, T, RandomEngine>(count, discrepancy_limit);
        LOG("");
        test_cosine_hemisphere<9, T, RandomEngine>(count, discrepancy_limit);
}
}

void test_cosine_hemisphere()
{
        test_cosine_hemisphere<double, std::mt19937_64>(10'000'000, 0.02);
}
}
