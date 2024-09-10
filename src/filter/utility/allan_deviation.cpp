/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "allan_deviation.h"

#include "instantiation.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ns::filter::utility
{
namespace
{
// sqrt(2 * ln(2) / pi)
template <typename T>
constexpr T BIAS_INSTABILITY_SCALING = 0.6642824702679600191174022L;

template <typename T>
void check(const std::vector<T>& data, const T frequency, const std::size_t output_count)
{
        if (data.size() < 3)
        {
                error("Data size (" + to_string(data.size()) + ") must be greater than or equal to 3");
        }

        if (output_count < 1)
        {
                error("Output count (" + to_string(output_count) + ") must be greater than or equal to 1");
        }

        if (!(frequency > 0))
        {
                error("Frequency (" + to_string(frequency) + ") must be greater than 0");
        }
}

template <typename T>
std::vector<std::size_t> log_space(const std::size_t max_m, const std::size_t output_count)
{
        ASSERT(max_m >= 1);
        ASSERT(output_count >= 1);

        std::vector<std::size_t> m;
        m.push_back(1);
        for (std::size_t i = 1; i <= output_count; ++i)
        {
                const T v = std::ceil(std::pow(static_cast<T>(max_m), i / static_cast<T>(output_count)));
                if (v > m.back())
                {
                        m.push_back(v);
                }
        }
        ASSERT(m.back() == max_m);
        return m;
}
}

template <typename T>
std::vector<AllanDeviation<T>> allan_deviation(
        const std::vector<T>& data,
        const T frequency,
        const std::size_t output_count)
{
        check(data, frequency, output_count);

        const T ts = 1 / frequency;
        const std::size_t max_m = (data.size() - 1) / 2;
        const std::vector<std::size_t> m = log_space<T>(max_m, output_count);

        std::vector<AllanDeviation<T>> res(m.size());

        for (std::size_t i = 0; i < m.size(); ++i)
        {
                res[i].tau = m[i] * ts;

                T sum = 0;
                ASSERT(data.size() > 2 * m[i]);
                const std::size_t count = data.size() - 2 * m[i];
                for (std::size_t j = 0; j < count; ++j)
                {
                        sum += square(data[j] - 2 * data[j + m[i]] + data[j + 2 * m[i]]);
                }

                res[i].deviation = std::sqrt(sum / (2 * square(res[i].tau) * count));
        }

        return res;
}

template <typename T>
BiasInstability<T> bias_instability(const std::vector<AllanDeviation<T>>& allan_deviation)
{
        if (allan_deviation.size() < 2)
        {
                error("Allan deviation size " + to_string(allan_deviation.size())
                      + " is too small for bias instability");
        }

        const std::size_t end = allan_deviation.size() - 1;
        std::size_t i = 0;
        for (; i < end; ++i)
        {
                if (allan_deviation[i].deviation <= allan_deviation[i + 1].deviation)
                {
                        break;
                }
        }

        ASSERT(i < allan_deviation.size());

        const T bias_instability = allan_deviation[i].deviation / BIAS_INSTABILITY_SCALING<T>;

        return {.bias_instability = bias_instability, .index = i};
}

template <typename T>
AngleRandomWalk<T> angle_random_walk(const std::vector<AllanDeviation<T>>& allan_deviation)
{
        if (allan_deviation.empty())
        {
                error("Allan deviation is empty");
        }

        if (allan_deviation.front().tau >= 1)
        {
                error("Allan deviation first tau (" + to_string(allan_deviation.front().tau) + ") must be less than 1");
        }

        for (std::size_t i = 1; i < allan_deviation.size(); ++i)
        {
                if (allan_deviation[i].tau >= 1)
                {
                        return {.angle_random_walk = allan_deviation[i].deviation, .index = i};
                }
        }

        error("Failed to determine angle random walk");
}

#define TEMPLATE(T)                                                                                     \
        template std::vector<AllanDeviation<T>> allan_deviation(const std::vector<T>&, T, std::size_t); \
        template BiasInstability<T> bias_instability(const std::vector<AllanDeviation<T>>&);            \
        template AngleRandomWalk<T> angle_random_walk(const std::vector<AllanDeviation<T>>&);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
