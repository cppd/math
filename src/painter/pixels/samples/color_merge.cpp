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

#include "color.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
ColorSamples<Color> merge_samples_sum_only(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        ASSERT(a.sum_only() && !b.empty());

        if (b.sum_only())
        {
                const auto create = [](const ColorSamples<Color>& min, const ColorSamples<Color>& max)
                {
                        return ColorSamples{
                                min.sum(),
                                max.sum(),
                                min.sum_weight(),
                                max.sum_weight(),
                                min.sum_contribution(),
                                max.sum_contribution()};
                };
                if (a.sum_contribution() < b.sum_contribution())
                {
                        return create(a, b);
                }
                return create(b, a);
        }

        ASSERT(b.full());
        if (a.sum_contribution() < b.min_contribution())
        {
                return {b.sum() + b.min(),
                        a.sum(),
                        b.max(),
                        b.sum_weight() + b.min_weight(),
                        a.sum_weight(),
                        b.max_weight(),
                        a.sum_contribution(),
                        b.max_contribution()};
        }
        if (a.sum_contribution() > b.max_contribution())
        {
                return {b.sum() + b.max(),
                        b.min(),
                        a.sum(),
                        b.sum_weight() + b.max_weight(),
                        b.min_weight(),
                        a.sum_weight(),
                        b.min_contribution(),
                        a.sum_contribution()};
        }
        return {a.sum() + b.sum(),
                b.min(),
                b.max(),
                a.sum_weight() + b.sum_weight(),
                b.min_weight(),
                b.max_weight(),
                b.min_contribution(),
                b.max_contribution()};
}

template <typename Color>
ColorSamples<Color> merge_samples_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        ASSERT(a.full() && b.full());

        Color sum = a.sum() + b.sum();
        typename Color::DataType sum_weight = a.sum_weight() + b.sum_weight();

        const ColorSamples<Color>* min = nullptr;
        const ColorSamples<Color>* max = nullptr;

        if (a.min_contribution() < b.min_contribution())
        {
                sum += b.min();
                sum_weight += b.min_weight();
                min = &a;
        }
        else
        {
                sum += a.min();
                sum_weight += a.min_weight();
                min = &b;
        }

        if (a.max_contribution() > b.max_contribution())
        {
                sum += b.max();
                sum_weight += b.max_weight();
                max = &a;
        }
        else
        {
                sum += a.max();
                sum_weight += a.max_weight();
                max = &b;
        }

        return {sum,
                min->min(),
                max->max(),
                sum_weight,
                min->min_weight(),
                max->max_weight(),
                min->min_contribution(),
                max->max_contribution()};
}
}

template <typename Color>
ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        if (a.full() && b.full())
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
        if (a.sum_only())
        {
                return merge_samples_sum_only(a, b);
        }
        if (b.sum_only())
        {
                return merge_samples_sum_only(b, a);
        }
        error("Failed to merge color samples");
}

#define TEMPLATE_C(C) template ColorSamples<C> merge_color_samples(const ColorSamples<C>&, const ColorSamples<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
