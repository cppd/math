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

#include <cstddef>
#include <vector>

namespace ns::filter::utility
{
namespace
{
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

        if (frequency > 0)
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
AllanDeviation<T> allan_deviation(const std::vector<T>& data, const T frequency, const std::size_t output_count)
{
        check(data, frequency, output_count);

        const T ts = 1 / frequency;

        const std::size_t max_m = (data.size() - 1) / 2;

        const std::vector<std::size_t> m = log_space<T>(max_m, output_count);

        AllanDeviation<T> res;

        res.taus.resize(m.size());
        res.deviations.resize(m.size());

        for (std::size_t i = 0; i < m.size(); ++i)
        {
                res.taus[i] = m[i] * ts;

                T sum = 0;
                const std::size_t count = data.size() - 2 * m[i];
                ASSERT(count < data.size());
                ASSERT(count > 0);
                for (std::size_t j = 0; j < count; ++j)
                {
                        ASSERT(j + 2 * m[i] < data.size());
                        sum += square(data[j] - 2 * data[j + m[i]] + data[j + 2 * m[i]]);
                }

                res.deviations[i] = std::sqrt(sum / (2 * square(res.taus[i]) * count));
        }

        return res;
}

#define TEMPLATE(T) template AllanDeviation<T> allan_deviation(const std::vector<T>&, T, std::size_t);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
