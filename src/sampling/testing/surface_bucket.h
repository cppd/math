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

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class Bucket final
{
        long long m_sample_count;
        long long m_uniform_count;
        long long m_pdf_count;
        double m_pdf_sum;

public:
        Bucket()
        {
                clear();
        }

        void clear()
        {
                m_sample_count = 0;
                m_uniform_count = 0;
                m_pdf_count = 0;
                m_pdf_sum = 0;
        }

        void add_sample()
        {
                ++m_sample_count;
        }

        long long sample_count() const
        {
                return m_sample_count;
        }

        void add_uniform()
        {
                ++m_uniform_count;
        }

        long long uniform_count() const
        {
                return m_uniform_count;
        }

        void add_pdf(double pdf)
        {
                m_pdf_count += 1;
                m_pdf_sum += pdf;
        }

        double pdf() const
        {
                if (!(m_pdf_count > 0))
                {
                        error("Bucket PDF not computed");
                }
                return m_pdf_sum / m_pdf_count;
        }

        void merge(const Bucket& bucket)
        {
                m_sample_count += bucket.m_sample_count;
                m_uniform_count += bucket.m_uniform_count;
                m_pdf_count += bucket.m_pdf_count;
                m_pdf_sum += bucket.m_pdf_sum;
        }
};

template <std::size_t N, typename T>
long long buckets_sample_count(const std::vector<Bucket<N, T>>& buckets)
{
        long long s = 0;
        for (const Bucket<N, T>& bucket : buckets)
        {
                s += bucket.sample_count();
        }
        return s;
}

template <std::size_t N, typename T>
long long buckets_uniform_count(const std::vector<Bucket<N, T>>& buckets)
{
        long long s = 0;
        for (const Bucket<N, T>& bucket : buckets)
        {
                s += bucket.uniform_count();
        }
        return s;
}

template <std::size_t N, typename T>
void check_bucket_sizes(const std::vector<Bucket<N, T>>& buckets)
{
        ASSERT(!buckets.empty());
        long long min = limits<long long>::max();
        long long max = limits<long long>::lowest();
        for (const Bucket<N, T>& bucket : buckets)
        {
                min = std::min(min, bucket.uniform_count());
                max = std::max(max, bucket.uniform_count());
        }
        long long maximum_max_min_ratio = N < 5 ? 3 : 10;
        if (max > 0 && min > 0 && max < maximum_max_min_ratio * min)
        {
                return;
        }
        std::ostringstream oss;
        oss << "Buckets max/min is too large" << '\n';
        oss << "max = " << max << '\n';
        oss << "min = " << min << '\n';
        oss << "max/min = " << (T(max) / min);
        error(oss.str());
}
}
