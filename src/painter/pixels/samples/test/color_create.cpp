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

#include "color_create.h"

#include "../../color_contribution.h"
#include "../color_create.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
void compare_colors(const std::vector<color::Color>& colors, const ColorSamples<color::Color>& samples)
{
        if (!(samples.count() == colors.size()))
        {
                error("Error sample count " + to_string(samples.count()) + " is not equal to color count "
                      + to_string(colors.size()));
        }

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (!(samples.color(i) == colors[i]))
                {
                        error("Sample color " + to_string(samples.color(i)) + " is not equal to " + to_string(colors[i])
                              + ", index " + to_string(i));
                }
        }
}

void compare_weights(const std::vector<color::Color::DataType>& weights, const ColorSamples<color::Color>& samples)
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

void compare_contributions(
        const std::vector<color::Color::DataType>& contributions,
        const ColorSamples<color::Color>& samples)
{
        if (!(samples.count() == contributions.size()))
        {
                error("Error sample count " + to_string(samples.count()) + " is not equal to contribution count "
                      + to_string(contributions.size()));
        }

        for (std::size_t i = 0; i < contributions.size(); ++i)
        {
                if (!(samples.contribution(i) == contributions[i]))
                {
                        error("Sample contribution " + to_string(samples.contribution(i)) + " is not equal to "
                              + to_string(contributions[i]) + ", index " + to_string(i));
                }
        }
}

void compare_weight_sum(const color::Color::DataType weight_sum, const ColorSamples<color::Color>& samples)
{
        if (!(samples.weight_sum() == weight_sum))
        {
                error("Sample weight sum " + to_string(samples.weight_sum()) + " is not equal to "
                      + to_string(weight_sum));
        }
}

void compare_color_sum(const color::Color& color_sum, const ColorSamples<color::Color>& samples)
{
        if (!(samples.color_sum() == color_sum))
        {
                error("Sample color sum " + to_string(samples.color_sum()) + " is not equal to "
                      + to_string(color_sum));
        }
}
}

void test_color_create()
{
        using T = color::Color::DataType;

        std::vector<std::optional<color::Color>> colors;
        std::vector<color::Color::DataType> weights;

        {
                colors = {};
                weights = {};
                const auto samples = create_color_samples(colors, weights);
                if (samples)
                {
                        error("Error creating empty");
                }
        }
        {
                colors = {color::Color(0.5)};
                weights = {1};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 1 sample");
                }
                compare_colors({color::Color(0.5)}, *samples);
                compare_weights({1}, *samples);
                compare_contributions({1 * sample_color_contribution(color::Color(0.5))}, *samples);
        }
        {
                colors = {color::Color(0.5), color::Color(0.25)};
                weights = {1, 1.1};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 2 samples");
                }
                compare_colors({T{1.1} * color::Color(0.25), T{1} * color::Color(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions(
                        {T{1.1} * sample_color_contribution(color::Color(0.25)),
                         T{1} * sample_color_contribution(color::Color(0.5))},
                        *samples);
        }
        {
                colors = {color::Color(0.5), color::Color(0.125), {}, color::Color(0.25)};
                weights = {1, 1.1, 10, 1.2};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 3 samples");
                }
                compare_colors({T{1.1} * color::Color(0.125), T{1} * color::Color(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions(
                        {T{1.1} * sample_color_contribution(color::Color(0.125)),
                         T{1} * sample_color_contribution(color::Color(0.5))},
                        *samples);
                compare_color_sum(T{1.2} * color::Color(0.25), *samples);
                compare_weight_sum(1.2, *samples);
        }
        {
                colors = {{}, color::Color(1), {}, color::Color(0.25), color::Color(0.5), color::Color(0.125)};
                weights = {10, 1, 10, 1.1, 1.2, 1.3};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 4 samples");
                }
                compare_colors({T{1.3} * color::Color(0.125), T{1} * color::Color(1)}, *samples);
                compare_weights({1.3, 1}, *samples);
                compare_contributions(
                        {T{1.3} * sample_color_contribution(color::Color(0.125)),
                         T{1} * sample_color_contribution(color::Color(1))},
                        *samples);
                compare_color_sum(T{1.1} * color::Color(0.25) + T{1.2} * color::Color(0.5), *samples);
                compare_weight_sum(T{1.1} + T{1.2}, *samples);
        }
}
}
