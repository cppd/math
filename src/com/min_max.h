/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "error.h"

#include <algorithm>
#include <array>
#include <span>

namespace ns
{
namespace min_max_implementation
{
template <bool COMPUTE_MIN, unsigned COUNT, typename T>
constexpr T min_max_value_array(const std::span<const T> p)
{
        ASSERT(!p.empty());
        ASSERT(p.size() % COUNT == 0);

        const T* ptr = p.data();

        std::array<T, COUNT> m;
        for (unsigned i = 0; i < COUNT; ++i)
        {
                m[i] = ptr[i];
        }
        ptr += COUNT;

        const T* const end = p.data() + p.size();
        while (ptr != end)
        {
                for (unsigned i = 0; i < COUNT; ++i)
                {
                        if constexpr (COMPUTE_MIN)
                        {
                                m[i] = std::min(m[i], ptr[i]);
                        }
                        else
                        {
                                m[i] = std::max(m[i], ptr[i]);
                        }
                }
                ptr += COUNT;
        }

        T cmp = m[0];
        for (unsigned i = 1; i < COUNT; ++i)
        {
                if constexpr (COMPUTE_MIN)
                {
                        cmp = std::min(cmp, m[i]);
                }
                else
                {
                        cmp = std::max(cmp, m[i]);
                }
        }

        return cmp;
}

template <bool COMPUTE_MIN, typename T>
constexpr T min_max_value(const std::span<const T> p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        // AVX, 256 bits
        constexpr unsigned COUNT = std::is_same_v<T, float> ? 8 : 4;

        const T* ptr = p.data();
        T cmp;

        if (p.size() >= 2 * COUNT)
        {
                const std::size_t count = (p.size() / COUNT) * COUNT;
                cmp = min_max_value_array<COMPUTE_MIN, COUNT>(std::span<const T>(p.data(), count));
                ptr += count;
        }
        else
        {
                cmp = *ptr++;
        }

        const T* const end = p.data() + p.size();
        while (ptr != end)
        {
                if constexpr (COMPUTE_MIN)
                {
                        cmp = std::min(cmp, *ptr++);
                }
                else
                {
                        cmp = std::max(cmp, *ptr++);
                }
        }

        return cmp;
}
}

template <typename T>
constexpr T min_value(const std::span<const T> p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
        if (p.empty())
        {
                error("No data for finding minimum value");
        }
        return min_max_implementation::min_max_value<true>(p);
}

template <typename T>
constexpr T max_value(const std::span<const T> p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
        if (p.empty())
        {
                error("No data for finding maximum value");
        }
        return min_max_implementation::min_max_value<false>(p);
}
}
