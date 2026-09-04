/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "view/write.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/filter/analysis/allan_deviation.h>
#include <src/filter/analysis/noise_parameters.h>
#include <src/test/test.h>

#include <cstddef>
#include <random>
#include <string_view>
#include <vector>

namespace ns::filter::analysis::test
{
namespace
{
template <typename T>
void check(const T bias_instability, const T angle_random_walk, const T rate_random_walk)
{
        if (!(bias_instability > T{0.032} && bias_instability < T{0.051}))
        {
                error("Bias instability (" + to_string(bias_instability) + ") is out of range");
        }

        if (!(angle_random_walk > T{0.084} && angle_random_walk < T{0.13}))
        {
                error("Angle random walk (" + to_string(angle_random_walk) + ") is out of range");
        }

        if (!(rate_random_walk > T{0.0075} && rate_random_walk < T{0.011}))
        {
                error("Rate random walk (" + to_string(rate_random_walk) + ") is out of range");
        }
}

template <typename T>
void test_impl()
{
        const std::size_t count = 100'000;
        const T frequency = 100;
        const std::size_t output_count = 500;

        const std::vector<T> data = [&]
        {
                PCG engine;
                std::normal_distribution<T> nd;
                std::vector<T> res(count);
                T sum = 0;
                for (std::size_t i = 0; i < count; ++i)
                {
                        const T time = i / frequency;
                        const T bias = T{1} / 100 + time / 1000;
                        const T speed = bias + nd(engine);
                        sum += speed / frequency;
                        res[i] = sum;
                }
                return res;
        }();

        const std::vector<AllanDeviation<T>> deviations = allan_deviation(data, frequency, output_count);

        const NoiseParameter<T> bi = bias_instability(deviations);
        const NoiseParameter<T> arw = angle_random_walk(deviations);
        const NoiseParameter<T> rrw = rate_random_walk(deviations);

        view::write_to_file(deviations, bi, arw, rrw);

        check(bi.value, arw.value, rrw.value);
}

void test()
{
        LOG("Test Allan deviation");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Allan deviation passed");
}

TEST_SMALL("Allan Deviation", test)
}
}
