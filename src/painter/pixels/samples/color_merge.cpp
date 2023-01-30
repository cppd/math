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

#include "color_merge.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
ColorSamples<Color> merge_two_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        static_assert(ColorSamples<Color>::size() == 2);

        ASSERT(a.count() == 1 && b.count() == 1);

        const auto create = [](const ColorSamples<Color>& min, const ColorSamples<Color>& max)
        {
                return ColorSamples<Color>{
                        {       min.color(0),        max.color(0)},
                        {      min.weight(0),       max.weight(0)},
                        {min.contribution(0), max.contribution(0)},
                        2
                };
        };

        if (a.contribution(0) < b.contribution(0))
        {
                return create(a, b);
        }

        return create(b, a);
}

template <typename Color>
ColorSamples<Color> merge_samples_one_sample(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        static_assert(ColorSamples<Color>::size() == 2);

        ASSERT(a.count() == 1 && !b.empty());

        if (b.count() == 1)
        {
                return merge_two_samples(a, b);
        }

        ASSERT(b.count() == 2);

        if (a.contribution(0) < b.contribution(0))
        {
                return {
                        (b.full() ? b.color_sum() : Color(0)) + b.color(0),
                        {       a.color(0),        b.color(1)},
                        (b.full() ? b.weight_sum() : 0) + b.weight(0),
                        {      a.weight(0),       b.weight(1)},
                        {a.contribution(0), b.contribution(1)}
                };
        }

        if (a.contribution(0) > b.contribution(1))
        {
                return {
                        (b.full() ? b.color_sum() : Color(0)) + b.color(1),
                        {       b.color(0),        a.color(0)},
                        (b.full() ? b.weight_sum() : 0) + b.weight(1),
                        {      b.weight(0),       a.weight(0)},
                        {b.contribution(0), a.contribution(0)}
                };
        }

        return {
                a.color(0) + (b.full() ? b.color_sum() : Color(0)),
                {       b.color(0),        b.color(1)},
                a.weight(0) + (b.full() ? b.weight_sum() : 0),
                {      b.weight(0),       b.weight(1)},
                {b.contribution(0), b.contribution(1)}
        };
}

template <typename Color>
Color samples_color_sum(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        if (a.full())
        {
                if (b.full())
                {
                        return a.color_sum() + b.color_sum();
                }
                return a.color_sum();
        }
        if (b.full())
        {
                return b.color_sum();
        }
        return Color(0);
}

template <typename Color>
typename Color::DataType samples_weight_sum(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        return (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);
}

template <typename Color, typename Copy, typename Sum>
void merge_low_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b, const Copy copy, const Sum sum)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        std::size_t a_i = 0;
        std::size_t b_i = 0;

        for (std::size_t i = 0; i < COUNT / 2; ++i)
        {
                if (a.contribution(a_i) < b.contribution(b_i))
                {
                        copy(i, a_i++, a);
                }
                else
                {
                        copy(i, b_i++, b);
                }
        }

        while (a_i < COUNT / 2)
        {
                sum(a_i++, a);
        }

        while (b_i < COUNT / 2)
        {
                sum(b_i++, b);
        }
}

template <typename Color, typename Copy, typename Sum>
void merge_high_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b, const Copy copy, const Sum sum)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        std::size_t a_i = COUNT - 1;
        std::size_t b_i = COUNT - 1;

        for (std::size_t i = COUNT - 1; i >= COUNT / 2; --i)
        {
                if (a.contribution(a_i) > b.contribution(b_i))
                {
                        copy(i, a_i--, a);
                }
                else
                {
                        copy(i, b_i--, b);
                }
        }

        while (a_i >= COUNT / 2)
        {
                sum(a_i--, a);
        }

        while (b_i >= COUNT / 2)
        {
                sum(b_i--, b);
        }
}

template <typename Color>
ColorSamples<Color> merge_samples_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        ASSERT(a.count() == COUNT);
        ASSERT(b.count() == COUNT);

        Color sum_color = samples_color_sum(a, b);
        typename Color::DataType sum_weight = samples_weight_sum(a, b);

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        const auto copy = [&](const std::size_t to, const std::size_t from, const ColorSamples<Color>& samples)
        {
                colors[to] = samples.color(from);
                weights[to] = samples.weight(from);
                contributions[to] = samples.contribution(from);
        };

        const auto sum = [&](const std::size_t index, const ColorSamples<Color>& samples)
        {
                sum_color += samples.color(index);
                sum_weight += samples.weight(index);
        };

        merge_low_full(a, b, copy, sum);
        merge_high_full(a, b, copy, sum);

        return {sum_color, colors, sum_weight, weights, contributions};
}
}

template <typename Color>
ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        static_assert(ColorSamples<Color>::size() == 2);

        if (a.count() == 2 && b.count() == 2)
        {
                return merge_samples_full(a, b);
        }
        if (a.empty())
        {
                return b;
        }
        if (b.empty())
        {
                return a;
        }
        if (a.count() == 1)
        {
                return merge_samples_one_sample(a, b);
        }
        if (b.count() == 1)
        {
                return merge_samples_one_sample(b, a);
        }
        error("Failed to merge color samples");
}

#define TEMPLATE_C(C) template ColorSamples<C> merge_color_samples(const ColorSamples<C>&, const ColorSamples<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
