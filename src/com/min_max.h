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

#include "error.h"

#include <span>

namespace ns
{
namespace min_max_implementation
{
template <bool COMPUTE_MIN, typename T>
constexpr T min_max_value(const std::span<const T>& p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        // AVX, 256 bits
        constexpr unsigned COUNT = std::is_same_v<T, float> ? 8 : 4;

        const T* ptr = p.data();
        T cmp;

        if (p.size() >= 2 * COUNT)
        {
                std::array<T, COUNT> m;
                for (unsigned i = 0; i < COUNT; ++i)
                {
                        m[i] = ptr[i];
                }
                ptr += COUNT;

                const T* end = p.data() + (p.size() / COUNT) * COUNT;
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

                cmp = m[0];
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
        }
        else
        {
                cmp = *ptr++;
        }

        const T* end = p.data() + p.size();
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
constexpr T min_value(const std::span<const T>& p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
        if (p.empty())
        {
                error("No data for finding minimum value");
        }
        return min_max_implementation::min_max_value<true>(p);
}

template <typename T>
constexpr T max_value(const std::span<const T>& p)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
        if (p.empty())
        {
                error("No data for finding maximum value");
        }
        return min_max_implementation::min_max_value<false>(p);
}
}
