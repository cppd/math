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

#pragma once

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/integrate.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ns::random
{
namespace sphere_buckets_implementation
{
template <std::size_t N, typename T>
T sphere_surface_area(std::type_identity_t<T> a, std::type_identity_t<T> b)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        // Assuming[Element[n,Integers]&&n>=0,Integrate[Sin[x]^n,x]]
        // -Cos[x] Hypergeometric2F1[1/2,(1-n)/2,3/2,Cos[x]^2] Sin[x]^(1+n) (Sin[x]^2)^(1/2 (-1-n))
        // For[i=2,i<=10,++i,f=Integrate[Sin[x]^(i-2),{x, a, b}];Print[i];Print[f]]
        // For[i=2,i<=10,++i,f=Simplify[Integrate[Sin[x]^(i-2),x]];Print[i];Print[f]]

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
                return (2 * b - 2 * a - std::sin(2 * b) + std::sin(2 * a)) / 4;
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
        if constexpr (N == 9)
        {
                return (1225 * std::cos(a) - 245 * std::cos(3 * a) + 49 * std::cos(5 * a) - 5 * std::cos(7 * a)
                        - 1225 * std::cos(b) + 245 * std::cos(3 * b) - 49 * std::cos(5 * b) + 5 * std::cos(7 * b))
                       / 2240;
        }
        if constexpr (N == 10)
        {
                return (-840 * a + 840 * b + 672 * std::sin(2 * a) - 168 * std::sin(4 * a) + 32 * std::sin(6 * a)
                        - 3 * std::sin(8 * a) - 672 * std::sin(2 * b) + 168 * std::sin(4 * b) - 32 * std::sin(6 * b)
                        + 3 * std::sin(8 * b))
                       / 3072;
        }
        if constexpr (N > 10)
        {
                return integrate<T>(
                        [](T x)
                        {
                                return std::pow(std::sin(x), N - 2);
                        },
                        a, b, /*count*/ 100);
        }
}
}

template <std::size_t N, typename T>
class SphereBuckets
{
        static constexpr unsigned SIZE = 2;
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
                namespace impl = sphere_buckets_implementation;

                m_distribution.clear();

                std::vector<T> distribution_values;
                distribution_values.reserve(m_buckets.size());

                for (auto& [angle, bucket] : m_buckets)
                {
                        T bucket_surface_area = impl::sphere_surface_area<N, T>(bucket.angle_from, bucket.angle_to);

                        Distribution& d = m_distribution.emplace_back();
                        d.angle_from = bucket.angle_from;
                        d.angle_to = bucket.angle_to;
                        d.distribution = bucket.sample_count / bucket_surface_area;

                        distribution_values.push_back(d.distribution);
                }

                m_buckets.clear();

                std::sort(distribution_values.begin(), distribution_values.end());
                T sum = 0;
                for (T d : distribution_values)
                {
                        sum += d;
                }
                sum *= to_radians(SIZE);
                for (Distribution& d : m_distribution)
                {
                        d.distribution /= sum;
                }

                std::sort(
                        m_distribution.begin(), m_distribution.end(),
                        [](const Distribution& d1, const Distribution& d2)
                        {
                                return d1.angle_from < d2.angle_from;
                        });
        }

        std::string histogram() const
        {
                constexpr int BAR_SIZE = 100;
                constexpr int DIVISION_SIZE = 10;

                std::ostringstream oss;
                oss << std::fixed;

                bool new_line = false;
                T max = limits<T>::lowest();
                for (const Distribution& d : m_distribution)
                {
                        max = std::max(max, d.distribution);
                }
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
                        oss << std::setprecision(1) << std::setw(5) << d.bucket_angle();
                        oss << ": " << std::setprecision(2) << std::setw(5) << d.distribution << ") ";
                        int count = std::round(d.distribution / max * BAR_SIZE);
                        for (int i = 0; i < count; i += DIVISION_SIZE)
                        {
                                oss << '+';
                                for (int j = i + 1; j < i + DIVISION_SIZE && j < count; ++j)
                                {
                                        oss << '*';
                                }
                        }
                }
                return oss.str();
        }

        template <typename PDF>
        void compare_with_pdf(const PDF& pdf) const
        {
                for (const Distribution& d : m_distribution)
                {
                        constexpr int PDF_INTEGRATE_COUNT = 100;

                        const T distribution_value = d.distribution;

                        const T pdf_mean_value =
                                numerical::integrate<T>(pdf, d.angle_from, d.angle_to, PDF_INTEGRATE_COUNT)
                                / (d.angle_to - d.angle_from);

                        if (!(pdf_mean_value >= 0 && distribution_value >= 0))
                        {
                                error("Number is not positive and not zero: distribution = "
                                      + to_string(distribution_value, 5) + ", PDF = " + to_string(pdf_mean_value, 5));
                        }

                        if (pdf_mean_value == distribution_value)
                        {
                                continue;
                        }

                        T discrepancy_abs = std::abs(pdf_mean_value - distribution_value);
                        if (discrepancy_abs <= T(0.05))
                        {
                                continue;
                        }

                        T discrepancy_rel = discrepancy_abs / std::max(pdf_mean_value, distribution_value);
                        if (discrepancy_rel <= T(0.05))
                        {
                                continue;
                        }

                        error("Angle interval = [" + to_string(to_degrees(d.angle_from), 5) + ", "
                              + to_string(to_degrees(d.angle_to), 5) + "], distribution = "
                              + to_string(distribution_value, 5) + ", PDF = " + to_string(pdf_mean_value, 5));
                }
        }
};
}
