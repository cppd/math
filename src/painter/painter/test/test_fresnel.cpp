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

#include "../fresnel.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cstddef>
#include <random>
#include <span>
#include <vector>

namespace ns::painter
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
        TimePoint start_time = time();
        for (std::size_t i = 0; i < data.size(); ++i)
        {
                result[i] = f(data[i]);
        }
        text += ": " + to_string_fixed(duration_from(start_time), 5);
        text += ", byte sum = " + to_string(byte_sum(result));
        LOG(text);
}

template <std::size_t N, typename T>
void test_fresnel_performance()
{
        constexpr int DATA_SIZE = 10'000'000;
        constexpr T N_1 = 1;
        constexpr T N_2 = 1.5;
        constexpr T ETA = N_1 / N_2;
        constexpr T K = T(0.5);

        LOG(space_name(N) + ", <" + type_name<T>() + ">");

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const Vector<N, T> normal = sampling::uniform_on_sphere<N, T>(engine);
        const std::vector<Vector<N, T>> data = random_data<N, T>(DATA_SIZE, engine);

        test("  fresnel d", data,
             [&](const Vector<N, T>& v)
             {
                     return fresnel_dielectric(v, normal, N_1, N_2);
             });

        test("  fresnel c", data,
             [&](const Vector<N, T>& v)
             {
                     return fresnel_conductor(v, normal, ETA, K);
             });
}

template <std::size_t N>
void test_fresnel_performance()
{
        test_fresnel_performance<N, float>();
        test_fresnel_performance<N, double>();
}

void test_fresnel()
{
        test_fresnel_performance<2>();
        test_fresnel_performance<3>();
        test_fresnel_performance<4>();
        test_fresnel_performance<5>();
}

TEST_PERFORMANCE("Fresnel", test_fresnel)
}
}
