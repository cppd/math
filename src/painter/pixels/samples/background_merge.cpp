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
BackgroundSamples<Color> merge_two_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(a.count() == 1 && b.count() == 1);

        if (a.weight(0) < b.weight(0))
        {
                return BackgroundSamples<Color>({a.weight(0), b.weight(0)}, 2);
        }

        return BackgroundSamples<Color>({b.weight(0), a.weight(0)}, 2);
}

template <typename Color>
BackgroundSamples<Color> merge_samples_one_sample(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(a.count() == 1 && !b.empty());

        if (b.count() == 1)
        {
                return merge_two_samples(a, b);
        }

        ASSERT(b.count() == 2);

        if (a.weight(0) < b.weight(0))
        {
                return BackgroundSamples<Color>(
                        (b.full() ? b.weight_sum() : 0) + b.weight(0), {a.weight(0), b.weight(1)});
        }

        if (a.weight(0) > b.weight(1))
        {
                return BackgroundSamples<Color>(
                        (b.full() ? b.weight_sum() : 0) + b.weight(1), {b.weight(0), a.weight(0)});
        }

        return BackgroundSamples<Color>(a.weight(0) + (b.full() ? b.weight_sum() : 0), {b.weight(0), b.weight(1)});
}

template <typename Color>
BackgroundSamples<Color> merge_samples_full(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(a.count() == 2 && b.count() == 2);

        typename Color::DataType sum = (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);

        const BackgroundSamples<Color>* min = nullptr;
        const BackgroundSamples<Color>* max = nullptr;

        if (a.weight(0) < b.weight(0))
        {
                sum += b.weight(0);
                min = &a;
        }
        else
        {
                sum += a.weight(0);
                min = &b;
        }

        if (a.weight(1) > b.weight(1))
        {
                sum += b.weight(1);
                max = &a;
        }
        else
        {
                sum += a.weight(1);
                max = &b;
        }

        return BackgroundSamples<Color>(sum, {min->weight(0), max->weight(1)});
}
}

template <typename Color>
BackgroundSamples<Color> merge_background_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

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
        error("Failed to merge background samples");
}

#define TEMPLATE_C(C)                                           \
        template BackgroundSamples<C> merge_background_samples( \
                const BackgroundSamples<C>&, const BackgroundSamples<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
