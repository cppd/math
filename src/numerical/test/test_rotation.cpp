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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/rotation.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>
#include <tuple>
#include <vector>

namespace ns::numerical
{
namespace
{
template <typename T>
Vector<3, T> random_vector(PCG& pcg)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        return {urd(pcg), urd(pcg), urd(pcg)};
}

template <typename T>
std::vector<std::tuple<T, Vector<3, T>>> random_rotation_vectors(const int count, PCG& pcg)
{
        std::uniform_real_distribution<T> urd_angle(-3 * PI<T>, 3 * PI<T>);
        std::vector<std::tuple<T, Vector<3, T>>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(urd_angle(pcg), random_vector<T>(pcg).normalized());
        }
        return res;
}

template <typename T>
std::vector<Vector<3, T>> random_vectors(const int count, PCG& pcg)
{
        std::vector<Vector<3, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(random_vector<T>(pcg));
        }
        return res;
}

template <int COUNT, typename T, typename F>
long long test(
        const std::vector<std::tuple<T, Vector<3, T>>>& rotation_vectors,
        const std::vector<Vector<3, T>>& vectors,
        const F& f)
{
        ASSERT(rotation_vectors.size() == vectors.size());
        const std::size_t size = rotation_vectors.size();
        const Clock::time_point start_time = Clock::now();
        for (int c = 0; c < COUNT; ++c)
        {
                for (std::size_t i = 0; i < size; ++i)
                {
                        do_not_optimize(f(rotation_vectors[i], vectors[i]));
                }
        }
        return std::llround(COUNT * (size / duration_from(start_time)));
}

template <typename T, int COUNT, bool JPL>
void test_rotation_performance()
{
        constexpr int DATA_SIZE = 10'000;

        PCG engine;

        const std::vector<std::tuple<T, Vector<3, T>>> data_rv = random_rotation_vectors<T>(DATA_SIZE, engine);
        const std::vector<Vector<3, T>> data_v = random_vectors<T>(DATA_SIZE, engine);

        std::ostringstream oss;
        oss << "Rotations <" << type_name<T>() << ", " << (JPL ? " JPL" : "!JPL") << ">:";

        {
                const auto p = test<COUNT>(
                        data_rv, data_v,
                        [&](const std::tuple<T, Vector<3, T>>& rv, const Vector<3, T>& v)
                        {
                                const auto q = rotation_vector_to_quaternion<JPL, QuaternionHJ>(
                                        std::get<0>(rv), std::get<1>(rv));
                                return rotate_vector(q, v);
                        });
                oss << " quaternion = " << to_string_digit_groups(p) << " o/s";
        }
        {
                const auto p = test<COUNT>(
                        data_rv, data_v,
                        [&](const std::tuple<T, Vector<3, T>>& rv, const Vector<3, T>& v)
                        {
                                const auto m = rotation_vector_to_matrix<JPL>(std::get<0>(rv), std::get<1>(rv));
                                return m * v;
                        });
                oss << ", matrix = " << to_string_digit_groups(p) << " o/s";
        }

        LOG(oss.str());
}

template <typename T, int COUNT>
void test_rotation_performance()
{
        test_rotation_performance<T, COUNT, false>();
        test_rotation_performance<T, COUNT, true>();
}

void test_performance()
{
        test_rotation_performance<float, 5000>();
        test_rotation_performance<double, 5000>();
        test_rotation_performance<long double, 1000>();
}

TEST_PERFORMANCE("Rotation", test_performance)
}
}
