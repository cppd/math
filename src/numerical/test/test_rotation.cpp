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
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/matrix.h>
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

template <typename T, bool JPL>
std::vector<QuaternionHJ<T, JPL>> random_rotation_quaternions(const int count, PCG& pcg)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<QuaternionHJ<T, JPL>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(QuaternionHJ<T, JPL>({urd(pcg), urd(pcg), urd(pcg)}, urd(pcg)).normalized());
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

template <int COUNT, std::size_t DATA_SIZE, typename F>
long long test(const F& f)
{
        const Clock::time_point start_time = Clock::now();
        for (int c = 0; c < COUNT; ++c)
        {
                for (std::size_t i = 0; i < DATA_SIZE; ++i)
                {
                        f(i);
                }
        }
        return std::llround(COUNT * (DATA_SIZE / duration_from(start_time)));
}

template <typename T, bool JPL>
void test_rotation_vector_performance()
{
        constexpr int COUNT = std::is_same_v<T, long double> ? 50'000 : 500'000;
        constexpr int DATA_SIZE = 100;

        PCG engine;

        const std::vector<std::tuple<T, Vector<3, T>>> data_rv = random_rotation_vectors<T>(DATA_SIZE, engine);
        const std::vector<Vector<3, T>> data_v = random_vectors<T>(DATA_SIZE, engine);

        std::ostringstream oss;
        oss << "Rotation vectors <" << type_name<T>() << ", " << (JPL ? " JPL" : "!JPL") << ">:";

        {
                const auto p = test<COUNT, DATA_SIZE>(
                        [&](const std::size_t i)
                        {
                                const QuaternionHJ<T, JPL> rq = rotation_vector_to_quaternion<QuaternionHJ<T, JPL>>(
                                        std::get<0>(data_rv[i]), std::get<1>(data_rv[i]));
                                do_not_optimize(rotate_vector(rq, data_v[i]));
                        });
                oss << " quaternion = " << to_string_digit_groups(p) << " o/s";
        }
        {
                const auto p = test<COUNT, DATA_SIZE>(
                        [&](const std::size_t i)
                        {
                                const auto rm = rotation_vector_to_matrix<JPL>(
                                        std::get<0>(data_rv[i]), std::get<1>(data_rv[i]));
                                do_not_optimize(rm * data_v[i]);
                        });
                oss << ", matrix = " << to_string_digit_groups(p) << " o/s";
        }

        LOG(oss.str());
}

template <typename T, bool JPL, int ROTATION_COUNT>
void test_rotation_quaternion_performance()
{
        constexpr int COUNT = std::is_same_v<T, long double> ? 50'000 : 500'000;
        constexpr int DATA_SIZE = 100;

        PCG engine;

        const std::vector<QuaternionHJ<T, JPL>> data_rq = random_rotation_quaternions<T, JPL>(DATA_SIZE, engine);
        const std::vector<Vector<3, T>> data_v = random_vectors<T>(ROTATION_COUNT * DATA_SIZE, engine);

        std::ostringstream oss;
        oss << "Rotation quaternions " << ROTATION_COUNT << " <" << type_name<T>() << ", " << (JPL ? " JPL" : "!JPL")
            << ">:";

        {
                const auto p = test<COUNT, DATA_SIZE>(
                        [&](const std::size_t i)
                        {
                                const QuaternionHJ<T, JPL>& rq = data_rq[i];
                                for (int r = 0; r < ROTATION_COUNT; ++r)
                                {
                                        do_not_optimize(rotate_vector(rq, data_v[ROTATION_COUNT * i + r]));
                                }
                        });
                oss << " quaternion = " << to_string_digit_groups(p) << " o/s";
        }
        {
                const auto p = test<COUNT, DATA_SIZE>(
                        [&](const std::size_t i)
                        {
                                const Matrix<3, 3, T> m = data_rq[i].rotation_matrix();
                                for (int r = 0; r < ROTATION_COUNT; ++r)
                                {
                                        do_not_optimize(m * data_v[ROTATION_COUNT * i + r]);
                                }
                        });
                oss << ", matrix = " << to_string_digit_groups(p) << " o/s";
        }

        LOG(oss.str());
}

template <typename T>
void test_rotation_vector_performance()
{
        test_rotation_vector_performance<T, false>();
        test_rotation_vector_performance<T, true>();
}

template <typename T, int ROTATION_COUNT>
void test_rotation_quaternion_performance()
{
        test_rotation_quaternion_performance<T, false, ROTATION_COUNT>();
        test_rotation_quaternion_performance<T, true, ROTATION_COUNT>();
}

void test_performance()
{
        test_rotation_vector_performance<float>();
        test_rotation_vector_performance<double>();
        test_rotation_vector_performance<long double>();
        LOG("---");
        test_rotation_quaternion_performance<float, 1>();
        test_rotation_quaternion_performance<double, 1>();
        test_rotation_quaternion_performance<long double, 1>();
        LOG("---");
        test_rotation_quaternion_performance<float, 2>();
        test_rotation_quaternion_performance<double, 2>();
        test_rotation_quaternion_performance<long double, 2>();
        LOG("---");
        test_rotation_quaternion_performance<float, 3>();
        test_rotation_quaternion_performance<double, 3>();
        test_rotation_quaternion_performance<long double, 3>();
}

TEST_PERFORMANCE("Rotation", test_performance)
}
}
