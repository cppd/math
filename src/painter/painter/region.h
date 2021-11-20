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

#include <algorithm>
#include <array>

namespace ns::painter
{
template <std::size_t N>
class Region final
{
        static std::array<int, N> max_values_for_size(const std::array<int, N>& size)
        {
                std::array<int, N> max;
                for (std::size_t i = 0; i < N; ++i)
                {
                        max[i] = size[i] - 1;
                }
                return max;
        }

        template <std::size_t I, typename F>
        static void traverse(
                const std::array<int, N>& min,
                const std::array<int, N>& max,
                std::array<int, N>& p,
                const F& f)
        {
                for (int i = min[I]; i <= max[I]; ++i)
                {
                        p[I] = i;
                        if constexpr (I + 1 < N)
                        {
                                traverse<I + 1>(min, max, p, f);
                        }
                        else
                        {
                                f(p);
                        }
                }
        }

        std::array<int, N> max_;
        int integer_radius_;

public:
        Region(const std::array<int, N>& size, const int integer_radius)
                : max_(max_values_for_size(size)), integer_radius_(integer_radius)
        {
        }

        template <typename F>
        void traverse(const std::array<int, N>& pixel, const F& f) const
        {
                std::array<int, N> min;
                std::array<int, N> max;
                for (std::size_t i = 0; i < N; ++i)
                {
                        min[i] = std::max(0, pixel[i] - integer_radius_);
                        max[i] = std::min(max_[i], pixel[i] + integer_radius_);
                }
                std::array<int, N> p;
                traverse<0>(min, max, p, f);
        }
};
}
