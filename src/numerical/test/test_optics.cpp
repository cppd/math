/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/optics.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ns::numerical
{
namespace
{
template <std::size_t N, typename T, typename RandomEngine>
std::vector<Vector<N, T>> random_data(const int count, RandomEngine& engine)
{
        std::vector<Vector<N, T>> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(sampling::uniform_on_sphere<N, T>(engine));
        }
        return res;
}

template <int COUNT, std::size_t N, typename T, typename F>
long long test(const std::vector<Vector<N, T>>& data, const F& f)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const Vector<N, T>& v : data)
                {
                        do_not_optimize(f(v));
                }
        }
        return std::llround(COUNT * (data.size() / duration_from(start_time)));
}

template <std::size_t N, typename T>
void test_optics_performance()
{
        constexpr int DATA_SIZE = 10'000;
        constexpr int COUNT = 10'000;
        constexpr T N_1 = 1;
        constexpr T N_2 = 1.5;
        constexpr T ETA = N_1 / N_2;

        PCG engine;

        const Vector<N, T> normal = sampling::uniform_on_sphere<N, T>(engine);
        const std::vector<Vector<N, T>> data = random_data<N, T>(DATA_SIZE, engine);

        std::ostringstream oss;
        oss << "Optics <" << N << ", " << type_name<T>() << ">:";

        {
                const auto p = test<COUNT>(
                        data,
                        [&](const Vector<N, T>& v)
                        {
                                return reflect(v, normal);
                        });
                oss << " reflect = " << to_string_digit_groups(p) << " o/s";
        }
        {
                const auto p = test<COUNT>(
                        data,
                        [&](const Vector<N, T>& v)
                        {
                                return refract(v, normal, ETA);
                        });
                oss << ", refract = " << to_string_digit_groups(p) << " o/s";
        }
        {
                const auto p = test<COUNT>(
                        data,
                        [&](const Vector<N, T>& v)
                        {
                                return refract2(v, normal, ETA);
                        });
                oss << ", refract2 = " << to_string_digit_groups(p) << " o/s";
        }

        LOG(oss.str());
}

template <typename T>
void test_optics_performance()
{
        test_optics_performance<2, T>();
        test_optics_performance<3, T>();
        test_optics_performance<4, T>();
        test_optics_performance<5, T>();
}

void test_performance()
{
        test_optics_performance<float>();
        test_optics_performance<double>();
}

//

template <std::size_t N, typename T>
bool cmp(const Vector<N, T>& v1, const Vector<N, T>& v2, T precision)
{
        return (v1 - v2).norm() <= precision;
}

template <typename T>
void test_optics_impl(T precision)
{
        LOG(std::string("Test optics, <") + type_name<T>() + ">");

        constexpr T ETA = 0.5;
        constexpr T ETA_MIRROR = 2;
        const Vector<2, T> v = Vector<2, T>(2, -1).normalized();
        const Vector<2, T> n = Vector<2, T>(0, 1).normalized();
        const Vector<2, T> reflected = Vector<2, T>(2, 1).normalized();
        const T sin2 = ETA * v[0];
        const Vector<2, T> refracted = Vector<2, T>(sin2, -std::sqrt(1 - square(sin2))).normalized();

        {
                const Vector<2, T> r = reflect(v, n);
                if (!cmp(r, reflected, precision))
                {
                        error("Error reflecting 1, " + to_string(r));
                }
        }
        {
                const Vector<2, T> r = reflect_vn(-v, n);
                if (!cmp(r, reflected, precision))
                {
                        error("Error reflecting 2, " + to_string(r));
                }
        }
        {
                const std::optional<Vector<2, T>> r = refract(v, n, ETA);
                if (!r)
                {
                        error("Error refracting 1, not refracted");
                }
                if (!cmp(*r, refracted, precision))
                {
                        error("Error refracting 1, " + to_string(*r));
                }
        }
        {
                const std::optional<Vector<2, T>> r = refract2(v, n, ETA);
                if (!r)
                {
                        error("Error refracting 2, not refracted");
                }
                if (!cmp(*r, refracted, precision))
                {
                        error("Error refracting 2, " + to_string(*r));
                }
        }
        if (refract(v, n, ETA_MIRROR))
        {
                error("Error refracting 1, refracted");
        }
        if (refract2(v, n, ETA_MIRROR))
        {
                error("Error refracting 2, refracted");
        }
}

void test_optics()
{
        test_optics_impl<float>(1e-7);
        test_optics_impl<double>(1e-15);
}

TEST_SMALL("Optics", test_optics)
TEST_PERFORMANCE("Optics", test_performance)
}
}
