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

#include <vector>

namespace ns::color
{
namespace average_implementation
{
template <typename Container>
void check_parameters(
        const Container& waves,
        const Container& samples,
        const typename Container::value_type from,
        const typename Container::value_type to,
        const std::size_t count)
{
        if (!(waves.size() == samples.size()))
        {
                error("Waves size " + to_string(waves.size()) + " is not equal to samples size "
                      + to_string(samples.size()));
        }

        if (!(waves.size() >= 2))
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
}
}

template <typename ResultType, typename Container>
std::vector<ResultType> average(
        const Container& waves,
        const Container& samples,
        const typename Container::value_type from,
        const typename Container::value_type to,
        const std::size_t count)
{
        using T = typename Container::value_type;
        static_assert(std::is_floating_point_v<T>);

        average_implementation::check_parameters(waves, samples, from, to, count);

        const auto area = [&](T a, T b, std::size_t i)
        {
                ASSERT(i > 0 && i < waves.size());
                ASSERT(b >= a && a >= waves[i - 1] && b <= waves[i]);

                T l = b - a;
                T p = a + (l / 2);
                T k = (p - waves[i - 1]) / (waves[i] - waves[i - 1]);
                T v = std::lerp(samples[i - 1], samples[i], k);
                return v * l;
        };

        std::size_t src_i = 0;

        const std::size_t dst_count = count + 1;
        std::size_t dst_i = 0;
        T dst_prev = limits<T>::max();
        T dst_wave = from;
        const auto dst_move = [&]()
        {
                ++dst_i;
                ASSERT(dst_i <= dst_count);
                if (dst_i < dst_count)
                {
                        T w = std::lerp(from, to, static_cast<T>(dst_i) / count);
                        ASSERT(w > dst_wave && w <= to);
                        dst_prev = dst_wave;
                        dst_wave = w;
                }
        };

        T prev_wave;
        if (waves[src_i] < dst_wave)
        {
                prev_wave = waves[src_i];
                ++src_i;
        }
        else
        {
                prev_wave = dst_wave;
                dst_move();
        }

        std::vector<ResultType> r;
        r.reserve(count);

        T sum = 0;

        while (src_i < waves.size() && dst_i < dst_count)
        {
                if (waves[src_i] < dst_wave)
                {
                        if (dst_i > 0 && src_i > 0)
                        {
                                sum += area(prev_wave, waves[src_i], src_i);
                        }
                        prev_wave = waves[src_i];
                        ++src_i;
                }
                else
                {
                        if (dst_i > 0 && src_i > 0)
                        {
                                sum += area(prev_wave, dst_wave, src_i);

                                ASSERT(dst_wave - dst_prev > 0);
                                r.push_back(sum / (dst_wave - dst_prev));
                                sum = 0;
                        }
                        else if (dst_i > 0 && src_i == 0)
                        {
                                r.push_back(0);
                        }
                        prev_wave = dst_wave;
                        dst_move();
                }
        }

        if (dst_i < dst_count)
        {
                if (dst_i > 0)
                {
                        ASSERT(dst_wave - dst_prev > 0);
                        r.push_back(sum / (dst_wave - dst_prev));
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
