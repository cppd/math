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
#include <src/com/print.h>

#include <array>
#include <vector>

namespace ns::color
{
template <typename ResultType, typename Container>
std::vector<ResultType> average(
        const Container& waves,
        const Container& samples,
        typename Container::value_type from,
        typename Container::value_type to,
        unsigned count)
{
        using T = typename Container::value_type;
        static_assert(std::is_floating_point_v<T>);

        if (waves.size() != samples.size())
        {
                error("Waves size " + to_string(waves.size()) + " is not equal to samples size "
                      + to_string(samples.size()));
        }

        if (waves.size() < 2)
        {
                error("Sample count " + to_string(waves.size()) + " is less than 2");
        }

        if (!std::is_sorted(waves.cbegin(), waves.cend()))
        {
                error("Sample waves are not sorted");
        }

        if (!(from < to))
        {
                error("The starting wavelength (" + to_string(from) + ") must be less than the ending wavelength ("
                      + to_string(to) + ")");
        }

        if (!(count >= 1))
        {
                error("Sample count " + to_string(count) + " must be positive");
        }

        std::vector<T> dst_waves;
        dst_waves.reserve(count + 1);
        dst_waves.push_back(from);
        for (unsigned i = 1; i <= count; ++i)
        {
                T wave = std::lerp(from, to, static_cast<T>(i) / count);
                ASSERT(wave > dst_waves.back() && wave <= to);
                dst_waves.push_back(wave);
        }

        std::size_t src = 0;
        std::size_t dst = 0;

        T prev;
        if (waves[src] < dst_waves[dst])
        {
                prev = waves[src];
                ++src;
        }
        else
        {
                prev = dst_waves[dst];
                ++dst;
        }

        std::vector<ResultType> r;
        r.reserve(count);

        T sum = 0;

        while (src < waves.size() && dst < dst_waves.size())
        {
                if (waves[src] < dst_waves[dst])
                {
                        if (dst > 0 && src > 0)
                        {
                                T p = (waves[src] + prev) / 2;
                                T k = (p - waves[src - 1]) / (waves[src] - waves[src - 1]);
                                T v = std::lerp(samples[src - 1], samples[src], k);
                                T l = waves[src] - prev;
                                sum += v * l;
                        }
                        prev = waves[src];
                        ++src;
                }
                else
                {
                        if (dst > 0 && src > 0)
                        {
                                T p = (dst_waves[dst] + prev) / 2;
                                T k = (p - waves[src - 1]) / (waves[src] - waves[src - 1]);
                                T v = std::lerp(samples[src - 1], samples[src], k);
                                T l = dst_waves[dst] - prev;
                                sum += v * l;

                                r.push_back(sum / (dst_waves[dst] - dst_waves[dst - 1]));
                                sum = 0;
                        }
                        else if (dst > 0 && src == 0)
                        {
                                r.push_back(0);
                        }
                        prev = dst_waves[dst];
                        ++dst;
                }
        }

        if (dst < dst_waves.size())
        {
                if (dst > 0)
                {
                        r.push_back(sum / (dst_waves[dst] - dst_waves[dst - 1]));
                }
                while (r.size() < count)
                {
                        r.push_back(0);
                }
        }

        ASSERT(r.size() == count);

        return r;
}
}
