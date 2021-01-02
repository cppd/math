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

#include "test_cosine_on_hemisphere.h"

#include "../sphere.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>

#include <map>
#include <sstream>

namespace ns::random
{
namespace
{
template <typename T>
T to_round_degrees(T angle)
{
        constexpr unsigned v = 5;
        static_assert(90 % v == 0);
        angle = std::floor(angle * 180 / PI<T> / v) * v;
        angle = std::clamp(angle, T(0), T(90 - v));
        return angle + v / T(2);
}

template <typename T>
T to_radians(T angle)
{
        return angle / 180 * PI<T>;
}

struct Sum
{
        double distribution = 0;
        double uniform = 0;
};

template <typename T>
void normalize(std::map<T, Sum>* buckets)
{
        double max = limits<double>::lowest();

        for (auto& [angle, sum] : *buckets)
        {
                sum.distribution /= sum.uniform;
                max = std::max(max, sum.distribution);
        }

        for (auto& [angle, sum] : *buckets)
        {
                sum.distribution /= max;
        }
}

template <typename T>
void print(const std::map<T, Sum>& buckets)
{
        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(1);
        for (const auto& [angle, sum] : buckets)
        {
                oss << std::setw(5) << angle;
                oss << ": " << std::string(sum.distribution * 100, '*');
                oss << '\n';
        }
        LOG(oss.str());
}

template <std::size_t N, typename T, typename RandomVector, typename PDF>
void test_distribution(
        const std::string& name,
        int count,
        std::mt19937_64& random_engine,
        const RandomVector& random_vector,
        const PDF& pdf)
{
        LOG("Test distribution " + name + " in " + space_name(N) + ", " + to_string_digit_groups(count) + ", "
            + type_name<T>());

        std::map<T, Sum> buckets;

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine);

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v = random_vector(normal);
                T cosine = dot(v, normal);
                cosine = std::clamp(cosine, T(0), T(1));
                T degrees = to_round_degrees(std::acos(cosine));
                ++buckets[degrees].distribution;
        }

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v = random_on_sphere<N, T>(random_engine);
                T cosine = dot(v, normal);
                if (cosine < 0)
                {
                        v = -v;
                        cosine = -cosine;
                }
                cosine = std::clamp(cosine, T(0), T(1));
                T degrees = to_round_degrees(std::acos(cosine));
                ++buckets[degrees].uniform;
        }

        normalize(&buckets);

        print(buckets);

        for (const auto& [angle, sum] : buckets)
        {
                T distribution_value = sum.distribution;
                T pdf_value = pdf(to_radians(angle));

                if (pdf_value == distribution_value)
                {
                        continue;
                }

                T discrepancy = pdf_value - distribution_value;
                discrepancy /= (pdf_value != 0) ? pdf_value : distribution_value;

                if (!(std::abs(discrepancy) <= T(0.1)))
                {
                        error("Angle = " + to_string(angle, 5) + ", distribution = " + to_string(distribution_value, 5)
                              + ", PDF = " + to_string(pdf_value, 5));
                }
        }
}

template <std::size_t N, typename T, typename RandomVector>
void test_speed(const std::string& name, int count, std::mt19937_64& random_engine, const RandomVector& random_vector)
{
        LOG("Test speed " + name + " in " + space_name(N) + ", " + to_string_digit_groups(count) + ", "
            + type_name<T>());

        static Vector<N, T> sink;

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine);

        TimePoint start_time = time();

        for (int i = 0; i < count; ++i)
        {
                sink = random_vector(normal);
        }

        LOG(to_string_digit_groups(std::lround(count / duration_from(start_time))) + " per second");
}

template <std::size_t N, typename T>
void test_cosine_on_hemisphere(int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        test_distribution<N, T>(
                "cosine_weighted", count, random_engine,
                [&](const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                },
                [](T angle)
                {
                        return std::cos(angle);
                });

        test_speed<N, T>(
                "cosine_weighted", count, random_engine,
                [&](const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                });
}

template <typename T>
void test_cosine_on_hemisphere()
{
        test_cosine_on_hemisphere<3, T>(10'000'000);
        LOG("");
        test_cosine_on_hemisphere<4, T>(30'000'000);
        LOG("");
        test_cosine_on_hemisphere<5, T>(100'000'000);
        LOG("");
        test_cosine_on_hemisphere<6, T>(300'000'000);
}
}

void test_cosine_on_hemisphere()
{
        test_cosine_on_hemisphere<float>();
        test_cosine_on_hemisphere<double>();
}
}
