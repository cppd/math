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

#pragma once

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SphereBucket final
{
        long long sample_count_;
        long long uniform_count_;
        long long pdf_count_;
        double pdf_sum_;

public:
        SphereBucket()
        {
                clear();
        }

        void clear()
        {
                sample_count_ = 0;
                uniform_count_ = 0;
                pdf_count_ = 0;
                pdf_sum_ = 0;
        }

        void add_sample()
        {
                ++sample_count_;
        }

        [[nodiscard]] long long sample_count() const
        {
                return sample_count_;
        }

        void add_uniform()
        {
                ++uniform_count_;
        }

        [[nodiscard]] long long uniform_count() const
        {
                return uniform_count_;
        }

        void add_pdf(const double pdf)
        {
                pdf_count_ += 1;
                pdf_sum_ += pdf;
        }

        [[nodiscard]] double pdf() const
        {
                if (!(pdf_count_ > 0))
                {
                        error("Bucket PDF not computed");
                }
                return pdf_sum_ / pdf_count_;
        }

        void merge(const SphereBucket& bucket)
        {
                sample_count_ += bucket.sample_count_;
                uniform_count_ += bucket.uniform_count_;
                pdf_count_ += bucket.pdf_count_;
                pdf_sum_ += bucket.pdf_sum_;
        }
};

template <std::size_t N, typename T>
long long buckets_sample_count(const std::vector<SphereBucket<N, T>>& buckets)
{
        long long s = 0;
        for (const SphereBucket<N, T>& bucket : buckets)
        {
                s += bucket.sample_count();
        }
        return s;
}

template <std::size_t N, typename T>
long long buckets_uniform_count(const std::vector<SphereBucket<N, T>>& buckets)
{
        long long s = 0;
        for (const SphereBucket<N, T>& bucket : buckets)
        {
                s += bucket.uniform_count();
        }
        return s;
}

template <std::size_t N, typename T>
void check_bucket_sizes(const std::vector<SphereBucket<N, T>>& buckets)
{
        ASSERT(!buckets.empty());

        long long min = Limits<long long>::max();
        long long max = Limits<long long>::lowest();
        for (const SphereBucket<N, T>& bucket : buckets)
        {
                min = std::min(min, bucket.uniform_count());
                max = std::max(max, bucket.uniform_count());
        }

        constexpr long long MAXIMUM_MAX_MIN_RATIO = N < 5 ? 3 : 10;
        if (max > 0 && min > 0 && max < MAXIMUM_MAX_MIN_RATIO * min)
        {
                return;
        }

        std::ostringstream oss;
        oss << "Buckets max/min is too large" << '\n';
        oss << "max = " << max << '\n';
        oss << "min = " << min << '\n';
        oss << "max/min = " << (static_cast<T>(max) / min);
        error(oss.str());
}
}
