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

#include "../optics.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cstddef>
#include <random>
#include <span>
#include <vector>

namespace ns::numerical
{
namespace
{
template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_data(int count, std::mt19937_64& engine)
{
        std::vector<Vector<N, T>> data;
        data.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                data.push_back(sampling::uniform_on_sphere<N, T>(engine));
        }
        return data;
}

template <typename T>
long long byte_sum(const T& data)
{
        long long s = 0;
        for (std::byte v : std::as_bytes(std::span(data)))
        {
                s += std::to_integer<int>(v);
        }
        return s;
}

template <std::size_t N, typename T, typename F>
void test(std::string text, const std::vector<Vector<N, T>> data, const F& f)
{
        static_assert(std::is_trivially_copyable_v<decltype(f(data[0]))>);
        std::vector<decltype(f(data[0]))> result(data.size());
        Clock::time_point start_time = Clock::now();
        for (std::size_t i = 0; i < data.size(); ++i)
        {
                result[i] = f(data[i]);
        }
        text += ": " + to_string_fixed(duration_from(start_time), 5);
        text += ", byte sum = " + to_string(byte_sum(result));
        LOG(text);
}

template <std::size_t N, typename T>
bool cmp(const Vector<N, T>& v1, const Vector<N, T>& v2, T precision)
{
        return (v1 - v2).norm() <= precision;
}

template <std::size_t N, typename T>
void test_optics_performance()
{
        constexpr int DATA_SIZE = 10'000'000;
        constexpr T N_1 = 1;
        constexpr T N_2 = 1.5;
        constexpr T ETA = N_1 / N_2;

        LOG(space_name(N) + ", <" + type_name<T>() + ">");

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const Vector<N, T> normal = sampling::uniform_on_sphere<N, T>(engine);
        const std::vector<Vector<N, T>> data = random_data<N, T>(DATA_SIZE, engine);

        test("  reflect  ", data,
             [&](const Vector<N, T>& v)
             {
                     return reflect(v, normal);
             });
        test("  refract  ", data,
             [&](const Vector<N, T>& v)
             {
                     return refract(v, normal, ETA);
             });

        test("  refract 2", data,
             [&](const Vector<N, T>& v)
             {
                     return refract2(v, normal, ETA);
             });
}

template <std::size_t N>
void test_optics_performance()
{
        test_optics_performance<N, float>();
        test_optics_performance<N, double>();
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
                Vector<2, T> r = reflect(v, n);
                if (!cmp(r, reflected, precision))
                {
                        error("Error reflecting 1, " + to_string(r));
                }
        }
        {
                Vector<2, T> r = reflect_vn(-v, n);
                if (!cmp(r, reflected, precision))
                {
                        error("Error reflecting 2, " + to_string(r));
                }
        }
        {
                std::optional<Vector<2, T>> r = refract(v, n, ETA);
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
                std::optional<Vector<2, T>> r = refract2(v, n, ETA);
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

void test_performance()
{
        test_optics_performance<2>();
        test_optics_performance<3>();
        test_optics_performance<4>();
        test_optics_performance<5>();
}

TEST_SMALL("Optics", test_optics)
TEST_PERFORMANCE("Optics", test_performance)
}
}
