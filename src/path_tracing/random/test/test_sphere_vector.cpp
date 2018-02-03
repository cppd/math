/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "test_sphere_vector.h"

#include "com/error.h"
#include "com/log.h"
#include "com/random/engine.h"
#include "com/random/vector.h"
#include "com/time.h"
#include "path_tracing/random/sphere_vector.h"

#include <map>

namespace
{
template <typename T>
T cos_to_angle(T cosine)
{
        return std::acos(cosine) / static_cast<T>(PI) * 180;
}

template <typename Map>
void normalize(Map* map)
{
        using T = typename Map::mapped_type;

        T max = std::numeric_limits<T>::lowest();

        for (const auto& i : *map)
        {
                max = std::max(max, i.second);
        }

        for (auto& i : *map)
        {
                i.second /= max;
        }
}

template <typename T, typename RandomEngine>
void test_distribution()
{
        LOG("Test Distribution...");

        constexpr T DISCRETIZATION = 100;
        constexpr int SAMPLE_COUNT = 10'000'000;
        constexpr T DISCREPANCY_LIMIT = 0.01;

        RandomEngineWithSeed<RandomEngine> random_engine;

        std::map<T, T, std::greater<T>> buckets;

        std::uniform_real_distribution<T> urd(-1, 1);
        Vector<3, T> normal = normalize(random_vector<3, T>(random_engine, urd));

        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
                Vector<3, T> random_vector = normalize(random_cosine_hemisphere_any_length(random_engine, normal));

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

        for (const auto & [ cosine, value ] : buckets)
        {
                T discrepancy = std::abs(value - cosine);

                if (discrepancy > DISCREPANCY_LIMIT)
                {
                        LOG("angle = " + to_string(cos_to_angle(cosine), 5) + ", cos = " + to_string(cosine, 5) +
                            ", value = " + to_string(value, 5) + ", " + to_string(discrepancy, 5));

                        error("Huge discrepancy");
                }
        }
}

template <typename T, typename RandomEngine>
void test_speed()
{
        LOG("Test Speed...");

        constexpr int COUNT = 10'000'000;

        RandomEngineWithSeed<RandomEngine> random_engine;

        std::uniform_real_distribution<T> urd(-1, 1);

        std::vector<Vector<3, T>> data;
        for (int i = 0; i < COUNT; ++i)
        {
                data.push_back(normalize(random_vector<3, T>(random_engine, urd)));
        }

        double start_time = time_in_seconds();

        // Сумма для того, чтобы избежать оптимизаций компилятора из-за неиспользуемого значения функции.
        Vector<3, T> sum(0);
        for (const Vector<3, T>& n : data)
        {
                sum += random_cosine_hemisphere_any_length(random_engine, n);
        }

        LOG("Time = " + to_string_fixed(time_in_seconds() - start_time, 5) + " seconds, sum = " + to_string(sum));
}

template <typename T, typename RandomEngine>
void test_sphere_vector()
{
        test_distribution<T, RandomEngine>();
        test_speed<T, RandomEngine>();
}
}

void test_sphere_vector()
{
        test_sphere_vector<double, std::mt19937_64>();
}
