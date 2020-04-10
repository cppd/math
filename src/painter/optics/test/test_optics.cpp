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

#include "test_optics.h"

#include "../optics.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/time.h>
#include <src/com/type/name.h>
#include <src/utility/random/engine.h>

#include <random>
#include <vector>

namespace painter
{
namespace
{
constexpr int COUNT = 10000000;

template <typename T>
constexpr T ETA = T(1) / T(1.5);

template <typename T>
constexpr Vector<3, T> VECTOR(0.1, -0.2, 0.3);

template <typename T>
std::vector<Vector<3, T>> random_data(int count)
{
        using RandomEngine =
                std::conditional_t<std::is_same_v<std::remove_cv<T>, float>, std::mt19937, std::mt19937_64>;

        RandomEngineWithSeed<RandomEngine> engine;
        std::uniform_real_distribution<T> urd(-1, 1);

        std::vector<Vector<3, T>> data;

        for (int i = 0; i < count; ++i)
        {
                Vector<3, T> v;

                do
                {
                        v = Vector<3, T>(urd(engine), urd(engine), urd(engine)).normalized();
                } while (!is_finite(v));

                data.push_back(v);
        }

        return data;
}

template <typename T>
T abs_sum(const Vector<3, T>& v)
{
        return std::abs(v[0]) + std::abs(v[1]) + std::abs(v[2]);
}

template <typename T>
void test_optics_performance(int count, const Vector<3, T>& normal_vector, T eta)
{
        const std::vector<Vector<3, T>> data = random_data<T>(count);

        const Vector<3, T> normal = normal_vector.normalized();

        ASSERT(is_finite(normal));

        {
                T time = time_in_seconds();

                Vector<3, T> t;
                T sum = 0;
                for (const Vector<3, T>& v : data)
                {
                        if (refract(v, normal, eta, &t))
                        {
                                sum += abs_sum(t);
                        }
                }

                LOG("refract  : " + to_string(time_in_seconds() - time) + ", sum = " + to_string(sum));
        }

        {
                T time = time_in_seconds();

                Vector<3, T> t;
                T sum = 0;
                for (const Vector<3, T>& v : data)
                {
                        if (refract2(v, normal, eta, &t))
                        {
                                sum += abs_sum(t);
                        }
                }

                LOG("refract 2: " + to_string(time_in_seconds() - time) + ", sum = " + to_string(sum));
        }
}

template <typename T>
void test_optics_performance()
{
        LOG(std::string("<") + type_name<T>() + ">");
        test_optics_performance<T>(COUNT, VECTOR<T>, ETA<T>);
}
}

void test_optics_performance()
{
        test_optics_performance<float>();
        test_optics_performance<double>();
        test_optics_performance<long double>();
}
}
