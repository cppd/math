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

#include "background_merge.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
BackgroundSamples<Color> merge_samples_empty(const BackgroundSamples<Color>& a)
{
        if (a.empty())
        {
                return BackgroundSamples<Color>{};
        }
        return a;
}

template <typename Color>
BackgroundSamples<Color> merge_samples_sum_only(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        ASSERT(a.sum_only() && !b.empty());

        if (b.sum_only())
        {
                if (a.sum_weight() < b.sum_weight())
                {
                        return {a.sum_weight(), b.sum_weight()};
                }
                return {b.sum_weight(), a.sum_weight()};
        }

        ASSERT(b.full());
        if (a.sum_weight() < b.min_weight())
        {
                return {b.sum_weight() + b.min_weight(), a.sum_weight(), b.max_weight()};
        }
        if (a.sum_weight() > b.max_weight())
        {
                return {b.sum_weight() + b.max_weight(), b.min_weight(), a.sum_weight()};
        }
        return {a.sum_weight() + b.sum_weight(), b.min_weight(), b.max_weight()};
}

template <typename Color>
BackgroundSamples<Color> merge_samples_full(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        ASSERT(a.full() && b.full());

        typename Color::DataType sum_weight = a.sum_weight() + b.sum_weight();

        const BackgroundSamples<Color>* min = nullptr;
        const BackgroundSamples<Color>* max = nullptr;

        if (a.min_weight() < b.min_weight())
        {
                sum_weight += b.min_weight();
                min = &a;
        }
        else
        {
                sum_weight += a.min_weight();
                min = &b;
        }

        if (a.max_weight() > b.max_weight())
        {
                sum_weight += b.max_weight();
                max = &a;
        }
        else
        {
                sum_weight += a.max_weight();
                max = &b;
        }

        return {sum_weight, min->min_weight(), max->max_weight()};
}
}

template <typename Color>
BackgroundSamples<Color> merge_background_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        if (a.full() && b.full())
        {
                return merge_samples_full(a, b);
        }
        if (a.empty())
        {
                return merge_samples_empty(b);
        }
        if (b.empty())
        {
                return merge_samples_empty(a);
        }
        if (a.sum_only())
        {
                return merge_samples_sum_only(a, b);
        }
        if (b.sum_only())
        {
                return merge_samples_sum_only(b, a);
        }
        error("Failed to merge background samples");
}

#define TEMPLATE_C(C)                                           \
        template BackgroundSamples<C> merge_background_samples( \
                const BackgroundSamples<C>&, const BackgroundSamples<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
