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

#include "background_create.h"

#include "../background_create.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
void compare_weights(const std::vector<color::Color::DataType>& weights, const BackgroundSamples<color::Color>& samples)
{
        if (!(samples.count() == weights.size()))
        {
                error("Error sample count " + to_string(samples.count()) + " is not equal to weight count "
                      + to_string(weights.size()));
        }

        for (std::size_t i = 0; i < weights.size(); ++i)
        {
                if (!(samples.weight(i) == weights[i]))
                {
                        error("Sample weight " + to_string(samples.weight(i)) + " is not equal to "
                              + to_string(weights[i]) + ", index " + to_string(i));
                }
        }
}

void compare_weight_sum(const color::Color::DataType weight_sum, const BackgroundSamples<color::Color>& samples)
{
        if (!(samples.weight_sum() == weight_sum))
        {
                error("Sample weight sum " + to_string(samples.weight_sum()) + " is not equal to "
                      + to_string(weight_sum));
        }
}
}

void test_background_create()
{
        std::vector<std::optional<color::Color>> colors;
        std::vector<color::Color::DataType> weights;

        {
                colors = {};
                weights = {};
                const auto samples = create_background_samples(colors, weights);
                if (samples)
                {
                        error("Error creating empty");
                }
        }
        {
                colors = {{}};
                weights = {1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 1 sample");
                }
                compare_weights({1}, *samples);
        }
        {
                colors = {{}, {}};
                weights = {2, 1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 2 samples");
                }
                compare_weights({1, 2}, *samples);
        }
        {
                colors = {{}, color::Color(1), {}, {}};
                weights = {3, 100, 1, 2};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 3 samples");
                }
                compare_weights({1, 3}, *samples);
                compare_weight_sum(2, *samples);
        }
        {
                colors = {color::Color(1), {}, {}, {}, color::Color(1), {}};
                weights = {100, 3, 2, 4, 100, 1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 4 samples");
                }
                compare_weights({1, 4}, *samples);
                compare_weight_sum(2 + 3, *samples);
        }
}
}
