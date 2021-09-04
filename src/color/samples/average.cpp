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

#include "average.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <cmath>

namespace ns::color
{
namespace
{
template <typename T>
void check_parameters(
        const std::span<const T>& waves,
        const std::span<const T>& samples,
        const T& from,
        const T& to,
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

        if (!std::is_sorted(waves.begin(), waves.end()))
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

template <typename T>
T compute_area(
        const std::span<const T>& waves,
        const std::span<const T>& samples,
        const T& a,
        const T& b,
        const std::size_t i)
{
        ASSERT(i > 0 && i < waves.size());
        ASSERT(b >= a && a >= waves[i - 1] && b <= waves[i]);

        const T l = b - a;
        const T p = a + (l / 2);
        const T k = (p - waves[i - 1]) / (waves[i] - waves[i - 1]);
        const T v = std::lerp(samples[i - 1], samples[i], k);
        return v * l;
}

template <typename T>
void move_dst(const T& from, const T& to, const std::size_t count, std::size_t& dst_i, T& dst_prev, T& dst_next)
{
        ASSERT(dst_i <= count);
        ++dst_i;
        if (dst_i <= count)
        {
                const T w = std::lerp(from, to, static_cast<T>(dst_i) / count);
                ASSERT(w > dst_next && w <= to);
                dst_prev = dst_next;
                dst_next = w;
        }
}
}

template <typename ResultType, typename T>
std::vector<ResultType> average(
        const std::span<const T>& waves,
        const std::span<const T>& samples,
        const std::type_identity_t<T>& from,
        const std::type_identity_t<T>& to,
        const std::size_t count)
{
        static_assert(std::is_floating_point_v<ResultType>);
        static_assert(std::is_floating_point_v<T>);

        static constexpr ResultType DEFAULT_VALUE = 0;

        check_parameters(waves, samples, from, to, count);

        if (to <= waves.front() || from >= waves.back())
        {
                return std::vector<ResultType>(count, DEFAULT_VALUE);
        }

        std::vector<ResultType> result;
        result.reserve(count);

        std::size_t dst_i = 0;
        T dst_prev;
        T dst_next = from;
        move_dst(from, to, count, dst_i, dst_prev, dst_next);

        while (dst_next <= waves.front())
        {
                result.push_back(DEFAULT_VALUE);
                move_dst(from, to, count, dst_i, dst_prev, dst_next);
        }
        ASSERT(dst_i <= count);

        std::size_t src_i = 1;
        while (waves[src_i] <= dst_prev)
        {
                ++src_i;
                ASSERT(src_i < waves.size());
        }

        T prev_wave = std::max(waves[src_i - 1], dst_prev);
        T sum = 0;

        while (src_i < waves.size() && dst_i <= count)
        {
                if (waves[src_i] < dst_next)
                {
                        sum += compute_area(waves, samples, prev_wave, waves[src_i], src_i);
                        prev_wave = waves[src_i];
                        ++src_i;
                }
                else
                {
                        sum += compute_area(waves, samples, prev_wave, dst_next, src_i);
                        ASSERT(dst_next - dst_prev > 0);
                        result.push_back(sum / (dst_next - dst_prev));
                        sum = 0;
                        prev_wave = dst_next;
                        move_dst(from, to, count, dst_i, dst_prev, dst_next);
                }
        }

        if (dst_i <= count)
        {
                ASSERT(dst_next - dst_prev > 0);
                result.push_back(sum / (dst_next - dst_prev));
                while (result.size() < count)
                {
                        result.push_back(DEFAULT_VALUE);
                }
        }

        ASSERT(result.size() == count);

        return result;
}

#define AVERAGE_INSTANTIATION(T1, T2)                                                                    \
        template std::vector<T1> average(                                                                \
                const std::span<const T2>&, const std::span<const T2>&, const std::type_identity_t<T2>&, \
                const std::type_identity_t<T2>&, const std::size_t);

AVERAGE_INSTANTIATION(float, float)
AVERAGE_INSTANTIATION(float, double)
AVERAGE_INSTANTIATION(double, float)
AVERAGE_INSTANTIATION(double, double)
}