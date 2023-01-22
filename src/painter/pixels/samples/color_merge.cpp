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
ColorSamples<Color> merge_samples_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        static_assert(ColorSamples<Color>::size() == 2);

        ASSERT(a.count() == 2 && b.count() == 2);

        Color sum_color = (a.full() ? a.color_sum() : Color(0)) + (b.full() ? b.color_sum() : Color(0));
        typename Color::DataType sum_weight = (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);

        const ColorSamples<Color>* min = nullptr;
        const ColorSamples<Color>* max = nullptr;

        if (a.contribution(0) < b.contribution(0))
        {
                sum_color += b.color(0);
                sum_weight += b.weight(0);
                min = &a;
        }
        else
        {
                sum_color += a.color(0);
                sum_weight += a.weight(0);
                min = &b;
        }

        if (a.contribution(1) > b.contribution(1))
        {
                sum_color += b.color(1);
                sum_weight += b.weight(1);
                max = &a;
        }
        else
        {
                sum_color += a.color(1);
                sum_weight += a.weight(1);
                max = &b;
        }

        return {
                sum_color,
                {       min->color(0),        max->color(1)},
                sum_weight,
                {      min->weight(0),       max->weight(1)},
                {min->contribution(0), max->contribution(1)}
        };
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
