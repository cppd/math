/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <cstddef>
#include <vector>

namespace ns::painter::pixels::samples::test
{
template <std::size_t COUNT, typename Color, template <std::size_t, typename> typename Samples>
void compare_weights(const std::vector<typename Color::DataType>& weights, const Samples<COUNT, Color>& samples)
{
        if (!(samples.count() == weights.size()))
        {
                error("Sample count " + to_string(samples.count()) + " is not equal to weight count "
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

template <std::size_t COUNT, typename Color, template <std::size_t, typename> typename Samples>
void compare_weight_sum(const typename Color::DataType weight_sum, const Samples<COUNT, Color>& samples)
{
        if (!(samples.weight_sum() == weight_sum))
        {
                error("Sample weight sum " + to_string(samples.weight_sum()) + " is not equal to "
                      + to_string(weight_sum));
        }
}

template <std::size_t COUNT, typename Color, template <std::size_t, typename> typename Samples>
void compare_colors(const std::vector<Color>& colors, const Samples<COUNT, Color>& samples)
{
        if (!(samples.count() == colors.size()))
        {
                error("Sample count " + to_string(samples.count()) + " is not equal to color count "
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

template <std::size_t COUNT, typename Color, template <std::size_t, typename> typename Samples>
void compare_color_sum(const Color& color_sum, const Samples<COUNT, Color>& samples)
{
        if (!(samples.color_sum() == color_sum))
        {
                error("Sample color sum " + to_string(samples.color_sum()) + " is not equal to "
                      + to_string(color_sum));
        }
}

template <std::size_t COUNT, typename Color, template <std::size_t, typename> typename Samples>
void compare_contributions(
        const std::vector<typename Color::DataType>& contributions,
        const Samples<COUNT, Color>& samples)
{
        if (!(samples.count() == contributions.size()))
        {
                error("Sample count " + to_string(samples.count()) + " is not equal to contribution count "
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
}
