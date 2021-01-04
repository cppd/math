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

#include <cmath>
#include <map>
#include <random>
#include <sstream>
#include <unordered_map>

namespace ns::random
{
namespace
{
template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

template <std::size_t N, typename T>
T sphere_surface_area(std::type_identity_t<T> a, std::type_identity_t<T> b)
{
        static_assert(std::is_floating_point_v<T>);

        // Integrate[Sin[x]^n, x]
        // -Cos[x] Hypergeometric2F1[1/2,(1-n)/2,3/2,Cos[x]^2] Sin[x]^(1+n) (Sin[x]^2)^(1/2 (-1-n))
        // Integrate[Sin[x]^2, {x, a, b}]

        if constexpr (N == 2)
        {
                return b - a;
        }
        if constexpr (N == 3)
        {
                return std::cos(a) - std::cos(b);
        }
        if constexpr (N == 4)
        {
                return (-a + b + std::cos(a) * std::sin(a) - std::cos(b) * sin(b)) / 2;
        }
        if constexpr (N == 5)
        {
                return (9 * std::cos(a) - std::cos(3 * a) - 9 * std::cos(b) + std::cos(3 * b)) / 12;
        }
        if constexpr (N == 6)
        {
                return (-12 * a + 12 * b + 8 * std::sin(2 * a) - std::sin(4 * a) - 8 * std::sin(2 * b)
                        + std::sin(4 * b))
                       / 32;
        }
        if constexpr (N == 7)
        {
                return (150 * std::cos(a) - 25 * std::cos(3 * a) + 3 * std::cos(5 * a) - 150 * std::cos(b)
                        + 25 * std::cos(3 * b) - 3 * std::cos(5 * b))
                       / 240;
        }
        if constexpr (N == 8)
        {
                return (-60 * a + 60 * b + 45 * std::sin(2 * a) - 9 * std::sin(4 * a) + std::sin(6 * a)
                        - 45 * std::sin(2 * b) + 9 * std::sin(4 * b) - std::sin(6 * b))
                       / 192;
        }
}

template <std::size_t N, typename T>
class Buckets
{
        static constexpr unsigned SIZE = 3;
        static_assert(90 % SIZE == 0);

        static T to_bucket_angle(T angle)
        {
                angle = std::floor(angle * (180 / PI<T>) / SIZE) * SIZE;
                angle = std::clamp(angle, T(0), T(180 - SIZE));
                return angle;
        }

        static T to_radians(T angle)
        {
                return angle * (PI<T> / 180);
        }

        static T to_degrees(T angle)
        {
                return angle * (180 / PI<T>);
        }

        template <typename PDF>
        static T integrate(const PDF& pdf, T angle_from, T angle_to)
        {
                constexpr int COUNT = 100;
                T offset = T(0.5) / COUNT * (angle_to - angle_from);
                T from = angle_from + offset;
                T to = angle_to + offset;
                std::vector<T> values(COUNT);
                for (int i = 0; i < COUNT; ++i)
                {
                        T x = std::lerp(from, to, T(i) / COUNT);
                        values[i] = pdf(x);
                }
                std::sort(values.begin(), values.end());
                T sum = 0;
                for (int i = 0; i < COUNT; ++i)
                {
                        sum += values[i];
                }
                return sum / COUNT;
        }

        struct Bucket
        {
                T angle_from;
                T angle_to;
                long long sample_count;
        };

        struct Distribution
        {
                T angle_from;
                T angle_to;
                T distribution;

                T bucket_angle() const
                {
                        return to_bucket_angle((angle_from + angle_to) / 2);
                }
        };

        std::unordered_map<T, Bucket> m_buckets;
        std::vector<Distribution> m_distribution;

public:
        void add(T angle)
        {
                T bucket_angle = to_bucket_angle(angle);
                auto iter = m_buckets.find(bucket_angle);
                if (iter != m_buckets.end())
                {
                        ++iter->second.sample_count;
                        return;
                }
                Bucket& s = m_buckets.try_emplace(bucket_angle).first->second;
                s.angle_from = to_radians(bucket_angle);
                s.angle_to = to_radians(bucket_angle + SIZE);
                s.sample_count = 1;
        }

        void normalize()
        {
                T max = limits<T>::lowest();

                m_distribution.clear();

                for (auto& [angle, bucket] : m_buckets)
                {
                        T bucket_surface_area = sphere_surface_area<N, T>(bucket.angle_from, bucket.angle_to);

                        Distribution& n = m_distribution.emplace_back();
                        n.angle_from = bucket.angle_from;
                        n.angle_to = bucket.angle_to;
                        n.distribution = bucket.sample_count / bucket_surface_area;

                        max = std::max(max, n.distribution);
                }

                m_buckets.clear();

                for (Distribution& d : m_distribution)
                {
                        d.distribution /= max;
                }

                std::sort(
                        m_distribution.begin(), m_distribution.end(),
                        [](const Distribution& d1, const Distribution& d2)
                        {
                                return d1.angle_from < d2.angle_from;
                        });
        }

        void print() const
        {
                std::ostringstream oss;
                oss << std::fixed;
                oss << std::setprecision(1);
                bool new_line = false;
                for (const Distribution& d : m_distribution)
                {
                        if (new_line)
                        {
                                oss << '\n';
                        }
                        else
                        {
                                new_line = true;
                        }
                        oss << std::setw(5) << d.bucket_angle();
                        oss << ": ";
                        int count = std::round(d.distribution * 100);
                        for (int i = 0; i < count; i += 10)
                        {
                                oss << '+';
                                for (int j = i + 1; j < i + 10 && j < count; ++j)
                                {
                                        oss << '*';
                                }
                        }
                }
                LOG(oss.str());
        }

        template <typename PDF>
        void compare_with_pdf(const PDF& pdf) const
        {
                constexpr T LIMIT = 0.05;

                for (const Distribution& d : m_distribution)
                {
                        const T distribution_value = d.distribution;
                        const T pdf_value = integrate(pdf, d.angle_from, d.angle_to);

                        if (pdf_value == distribution_value)
                        {
                                continue;
                        }

                        T discrepancy_abs = pdf_value - distribution_value;
                        if (std::abs(discrepancy_abs) < LIMIT)
                        {
                                continue;
                        }

                        T discrepancy_rel = discrepancy_abs / (pdf_value != 0 ? pdf_value : distribution_value);
                        if (!(std::abs(discrepancy_rel) <= LIMIT))
                        {
                                error("Angle interval = [" + to_string(to_degrees(d.angle_from), 5) + ", "
                                      + to_string(to_degrees(d.angle_to), 5) + "], distribution = "
                                      + to_string(distribution_value, 5) + ", PDF = " + to_string(pdf_value, 5));
                        }
                }
        }
};

template <std::size_t N, typename T, typename RandomVector>
void test_unit(
        const std::string& name,
        long long count,
        RandomEngine<T>& random_engine,
        const RandomVector& random_vector)
{
        LOG(name + "\n  test unit in " + space_name(N) + ", " + to_string_digit_groups(count) + ", " + type_name<T>());

        for (long long i = 0; i < count; ++i)
        {
                Vector<N, T> normal = random_on_sphere<N, T>(random_engine);

                T normal_norm = normal.norm();
                if (!(normal_norm >= T(0.999) && normal_norm <= T(1.001)))
                {
                        error("Random on sphere normal is not unit " + to_string(normal_norm));
                }

                T norm = random_vector(normal).norm();
                if (!(norm >= T(0.999) && norm <= T(1.001)))
                {
                        error(name + " normal is not unit " + to_string(norm));
                }
        }
}

template <std::size_t N, typename T, typename RandomVector, typename PDF>
void test_distribution(
        const std::string& name,
        long long count,
        RandomEngine<T>& random_engine,
        const RandomVector& random_vector,
        const PDF& pdf)
{
        LOG(name + "\n  test distribution in " + space_name(N) + ", " + to_string_digit_groups(count) + ", "
            + type_name<T>());

        Buckets<N, T> buckets;

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine).normalized();

        for (long long i = 0; i < count; ++i)
        {
                Vector<N, T> v = random_vector(normal).normalized();
                T cosine = dot(v, normal);
                cosine = std::clamp(cosine, T(-1), T(1));
                buckets.add(std::acos(cosine));
        }

        buckets.normalize();
        buckets.print();
        buckets.compare_with_pdf(pdf);
}

template <std::size_t N, typename T, typename RandomVector>
void test_speed(
        const std::string& name,
        long long count,
        RandomEngine<T>& random_engine,
        const RandomVector& random_vector)
{
        LOG(name + "\n  test speed in " + space_name(N) + ", " + to_string_digit_groups(count) + ", " + type_name<T>());

        static Vector<N, T> sink;

        const Vector<N, T> normal = random_on_sphere<N, T>(random_engine);

        TimePoint start_time = time();

        for (long long i = 0; i < count; ++i)
        {
                sink = random_vector(normal);
        }

        LOG("  " + to_string_digit_groups(std::lround(count / duration_from(start_time))) + " per second");
}

template <std::size_t N, typename T>
void test_uniform_on_sphere(long long count)
{
        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const std::string name = "uniform";

        test_unit<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                });

        test_distribution<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                },
                [](T angle) -> T
                {
                        if (angle >= 0 && angle < (PI<T>))
                        {
                                return 1;
                        }
                        return T(0);
                });

        test_speed<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& /*normal*/)
                {
                        return random_on_sphere<N, T>(random_engine);
                });
}

template <std::size_t N, typename T>
void test_cosine_on_hemisphere(long long count)
{
        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const std::string name = "cosine_weighted";

        test_unit<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                });

        test_distribution<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                },
                [](T angle) -> T
                {
                        if (angle >= 0 && angle < (PI<T> / 2))
                        {
                                return std::cos(angle);
                        }
                        return T(0);
                });

        test_speed<N, T>(
                name, count, random_engine,
                [&](const Vector<N, T>& normal)
                {
                        return random_cosine_weighted_on_hemisphere(random_engine, normal);
                });
}

template <std::size_t N, typename T>
void test_distribution(long long count)
{
        test_uniform_on_sphere<N, T>(count);
        LOG("");
        test_cosine_on_hemisphere<N, T>(count);
}

template <typename T>
void test_distribution()
{
        test_distribution<3, T>(50'000'000);
        LOG("");
        test_distribution<4, T>(100'000'000);
        LOG("");
        test_distribution<5, T>(200'000'000);
        LOG("");
        test_distribution<6, T>(300'000'000);
}
}

void test_distribution_on_sphere()
{
        test_distribution<float>();
        LOG("");
        test_distribution<double>();
}
}
