/*
Copyright (C) 2017-2025 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received v1 copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::numerical
{
namespace median_implementation
{
template <typename T>
std::optional<T> compute(
        const std::vector<T>& v1,
        const std::vector<T>& v2,
        const std::size_t s1,
        const std::size_t s2,
        std::size_t& l,
        std::size_t& h)
{
        const std::size_t m1 = (l + h) / 2;
        const std::size_t m2 = (s1 + s2 + 1) / 2 - m1;

        ASSERT(m1 <= s1);
        const T l1 = (m1 == 0 ? Limits<T>::lowest() : v1[m1 - 1]);
        const T r1 = (m1 == s1 ? Limits<T>::max() : v1[m1]);

        ASSERT(m2 <= s2);
        const T l2 = (m2 == 0 ? Limits<T>::lowest() : v2[m2 - 1]);
        const T r2 = (m2 == s2 ? Limits<T>::max() : v2[m2]);

        if (l1 <= r2 && l2 <= r1)
        {
                if ((s1 + s2) % 2 == 0)
                {
                        return (std::max(l1, l2) + std::min(r1, r2)) / 2;
                }
                return std::max(l1, l2);
        }

        if (l1 > r2)
        {
                h = m1 - 1;
        }
        else
        {
                l = m1 + 1;
        }

        if (l > h)
        {
                error("Median not found");
        }

        return std::nullopt;
}
}

template <typename T>
[[nodiscard]] T median_of_sorted_data(const std::vector<T>& v1, const std::vector<T>& v2)
{
        namespace impl = median_implementation;

        static_assert(std::is_floating_point_v<T>);

        if (v1.empty() && v2.empty())
        {
                error("No data for median");
        }

        const std::size_t s1 = v1.size();
        const std::size_t s2 = v2.size();

        if (s1 > s2)
        {
                return median_of_sorted_data(v2, v1);
        }

        std::size_t l = 0;
        std::size_t h = s1;

        while (true)
        {
                if (const auto r = impl::compute(v1, v2, s1, s2, l, h))
                {
                        return *r;
                }
        }
}
}
