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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/settings/dimensions.h>
#include <src/shading/ggx/f1_albedo.h>
#include <src/test/test.h>

#include <cstddef>
#include <utility>

namespace ns::shading::ggx
{
namespace
{
constexpr int MAX = 10;

template <std::size_t N, typename T>
void test_cosine_weighted_average()
{
        for (int i = 0; i <= MAX; ++i)
        {
                const T roughness = static_cast<T>(i) / MAX;
                const T average = f1_albedo_cosine_weighted_average<N, T>(roughness);
                if (!(average > 0 && average <= 1))
                {
                        error("GGX F1 cosine-weighted average " + to_string(average)
                              + " is not in the range (0, 1] for roughness " + to_string(roughness));
                }
        }
}

template <std::size_t N, typename T>
void test_zero()
{
        for (int i = 1; i <= MAX; ++i)
        {
                static constexpr T ROUGHNESS = 0;
                const T cosine = static_cast<T>(i) / MAX;
                const T albedo = f1_albedo<N, T>(ROUGHNESS, cosine);
                if (!(albedo == 1))
                {
                        error("GGX F1 albedo " + to_string(albedo) + " is not equal to 1 for roughness "
                              + to_string(ROUGHNESS) + " and cosine " + to_string(cosine));
                }
        }

        for (int i = 1; i <= MAX; ++i)
        {
                static constexpr T COSINE = 0;
                const T roughness = static_cast<T>(i) / MAX;
                const T albedo = f1_albedo<N, T>(roughness, COSINE);
                if (!(albedo < 1))
                {
                        error("GGX F1 albedo " + to_string(albedo) + " is not less than 1 for roughness "
                              + to_string(roughness) + " and cosine " + to_string(COSINE));
                }
        }
}

template <std::size_t N, typename T>
void test_all()
{
        for (int i = 0; i <= MAX; ++i)
        {
                const T roughness = static_cast<T>(i) / MAX;
                for (int j = 0; j <= MAX; ++j)
                {
                        const T cosine = static_cast<T>(j) / MAX;
                        const T albedo = f1_albedo<N, T>(roughness, cosine);
                        if (!(albedo >= 0 && albedo <= 1))
                        {
                                error("GGX F1 albedo " + to_string(albedo)
                                      + " is not in the range [0, 1] for roughness " + to_string(roughness)
                                      + " and cosine " + to_string(cosine));
                        }
                }
        }
}

template <std::size_t N, typename T>
void test()
{
        test_cosine_weighted_average<N, T>();
        test_zero<N, T>();
        test_all<N, T>();
}

template <std::size_t N>
void test()
{
        test<N, float>();
        test<N, double>();
}

void test_albedo()
{
        []<std::size_t... I>(std::index_sequence<I...>&&)
        {
                (test<I>(), ...);
        }(settings::Dimensions());
}

TEST_SMALL("GGX F1 Albedo", test_albedo)
}
}
