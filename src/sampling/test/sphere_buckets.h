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
#include <src/geometry/shapes/sphere_surface.h>
#include <src/numerical/integrate.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace ns::sampling
{
template <std::size_t N, typename T>
class SphereBuckets
{
        static constexpr int BUCKET_COUNT = 90;
        static constexpr T BUCKET_SIZE = PI<T> / BUCKET_COUNT;
        static constexpr T BUCKETS_PER_RADIAN = BUCKET_COUNT / PI<T>;

        static T to_degrees(T angle)
        {
                return angle * (180 / PI<T>);
        }

        struct Distribution
        {
                T angle_from;
                T angle_to;
                T distribution;
        };

        std::vector<long long> m_buckets;
        std::vector<Distribution> m_distribution;

public:
        static T bucket_size()
        {
                return BUCKET_SIZE;
        }

        SphereBuckets()
        {
                m_buckets.resize(BUCKET_COUNT, 0);
        }

        void merge(const SphereBuckets& other)
        {
                ASSERT(m_buckets.size() == other.m_buckets.size());
                for (unsigned i = 0; i < m_buckets.size(); ++i)
                {
                        m_buckets[i] += other.m_buckets[i];
                }
        }

        void add(T angle)
        {
                int bucket = angle * BUCKETS_PER_RADIAN;
                bucket = std::clamp(bucket, 0, BUCKET_COUNT - 1);
                ++m_buckets[bucket];
        }

        void compute_distribution()
        {
                m_distribution.clear();

                std::vector<T> distribution_values;
                distribution_values.reserve(m_buckets.size());

                const long double SPHERE_K =
                        geometry::sphere_area(N) / geometry::sphere_relative_area<N, long double>(0, PI<T>);
                long long cnt = 0;
                for (unsigned bucket = 0; bucket < m_buckets.size(); ++bucket)
                {
                        Distribution& d = m_distribution.emplace_back();

                        cnt += m_buckets[bucket];

                        d.angle_from = bucket * BUCKET_SIZE;
                        d.angle_to = (bucket + 1) * BUCKET_SIZE;

                        long double bucket_area =
                                SPHERE_K * geometry::sphere_relative_area<N, long double>(d.angle_from, d.angle_to);
                        d.distribution = m_buckets[bucket] / bucket_area;

                        distribution_values.push_back(d.distribution);
                }

                for (Distribution& d : m_distribution)
                {
                        d.distribution /= cnt;
                }

                ASSERT(std::is_sorted(
                        m_distribution.begin(), m_distribution.end(),
                        [](const Distribution& d1, const Distribution& d2)
                        {
                                return d1.angle_from < d2.angle_from;
                        }));
        }

        std::string histogram() const
        {
                constexpr int BAR_SIZE = 100;
                constexpr int DIVISION_SIZE = 10;

                if (m_distribution.empty())
                {
                        error("There is no distribution");
                }

                T max = limits<T>::lowest();
                for (const Distribution& d : m_distribution)
                {
                        max = std::max(max, d.distribution);
                }

                std::ostringstream oss;

                bool new_line = false;
                for (const Distribution& d : m_distribution)
                {
                        if (!(d.distribution >= 0))
                        {
                                error("Number is not positive and not zero: distribution = "
                                      + to_string(d.distribution, 5));
                        }

                        if (new_line)
                        {
                                oss << '\n';
                        }
                        else
                        {
                                new_line = true;
                        }

                        oss << std::fixed << std::setprecision(1) << std::setw(5) << to_degrees(d.angle_from);
                        oss << ": ";
                        oss << std::scientific << std::setprecision(2) << d.distribution;
                        oss << " ";

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
