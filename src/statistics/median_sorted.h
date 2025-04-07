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

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::statistics
{
namespace median_array_implementation
{
template <typename T, typename F1, typename F2>
std::optional<T> compute(
        const F1& f1,
        const F2& f2,
        const std::size_t s1,
        const std::size_t s2,
        std::size_t& l,
        std::size_t& h)
{
        const std::size_t m1 = (l + h) / 2;
        const std::size_t m2 = (s1 + s2 + 1) / 2 - m1;

        ASSERT(m1 <= s1);
        const T l1 = (m1 == 0 ? Limits<T>::lowest() : f1(m1 - 1));
        const T r1 = (m1 == s1 ? Limits<T>::max() : f1(m1));

        ASSERT(m2 <= s2);
        const T l2 = (m2 == 0 ? Limits<T>::lowest() : f2(m2 - 1));
        const T r2 = (m2 == s2 ? Limits<T>::max() : f2(m2));

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

template <typename F1, typename F2>
[[nodiscard]] std::remove_cvref_t<decltype(std::declval<F1>()(0))> median_of_sorted_data(
        // T f(std::size_t i)
        const F1& f1,
        const std::size_t size_1,
        // T f(std::size_t i)
        const F2& f2,
        const std::size_t size_2)
{
        namespace impl = median_array_implementation;

        static_assert(std::is_same_v<decltype(f1(std::size_t{0})), decltype(f2(std::size_t{0}))>);
        using T = std::remove_cvref_t<decltype(f1(std::size_t{0}))>;
        static_assert(std::is_floating_point_v<T>);

        if (size_1 == 0 && size_2 == 0)
        {
                error("No data for median");
        }

        if (size_1 > size_2)
        {
                return median_of_sorted_data(f2, size_2, f1, size_1);
        }

        std::size_t l = 0;
        std::size_t h = size_1;

        while (true)
        {
                if (const auto r = impl::compute<T>(f1, f2, size_1, size_2, l, h))
                {
                        return *r;
                }
        }
}
}
