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

#include "com/merge.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
typename Color::DataType samples_weight_sum(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        return (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_samples_full(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        typename Color::DataType sum_weight = samples_weight_sum(a, b);
        std::array<typename Color::DataType, COUNT> weights;

        com::merge_full(
                a, b,
                [](const BackgroundSamples<Color>& samples, const std::size_t index)
                {
                        return samples.weight(index);
                },
                [&](const std::size_t to, const std::size_t from, const BackgroundSamples<Color>& samples)
                {
                        ASSERT(to < COUNT);
                        weights[to] = samples.weight(from);
                },
                [&](const std::size_t index, const BackgroundSamples<Color>& samples)
                {
                        sum_weight += samples.weight(index);
                });

        return BackgroundSamples<Color>(sum_weight, weights);
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_samples_partial(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        typename Color::DataType sum_weight = samples_weight_sum(a, b);
        std::array<typename Color::DataType, COUNT> weights;

        com::merge_partial(
                a, b,
                [](const BackgroundSamples<Color>& samples, const std::size_t index)
                {
                        return samples.weight(index);
                },
                [&](const std::size_t to, const std::size_t from, const BackgroundSamples<Color>& samples)
                {
                        ASSERT(to < COUNT);
                        weights[to] = samples.weight(from);
                },
                [&](const std::size_t index, const BackgroundSamples<Color>& samples)
                {
                        sum_weight += samples.weight(index);
                });

        return BackgroundSamples<Color>(sum_weight, weights);
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_samples(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        const std::size_t count = a.count() + b.count();

        std::array<typename Color::DataType, COUNT> weights;

        com::merge(
                a, b,
                [](const BackgroundSamples<Color>& samples, const std::size_t index)
                {
                        return samples.weight(index);
                },
                [&](const std::size_t to, const std::size_t from, const BackgroundSamples<Color>& samples)
                {
                        ASSERT(to < count);
                        weights[to] = samples.weight(from);
                });

        return BackgroundSamples<Color>(weights, count);
}
}

template <typename Color>
BackgroundSamples<Color> merge_background_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        const std::size_t a_count = a.count();
        const std::size_t b_count = b.count();

        if (a_count == COUNT && b_count == COUNT)
        {
                return merge_samples_full(a, b);
        }
        if (a_count == 0)
        {
                return b;
        }
        if (b_count == 0)
        {
                return a;
        }
        if (a_count + b_count <= COUNT)
        {
                return merge_samples(a, b);
        }
        if (a_count + b_count < 2 * COUNT)
        {
                return merge_samples_partial(a, b);
        }
        error("Failed to merge background samples");
}

#define TEMPLATE_C(C)                                           \
        template BackgroundSamples<C> merge_background_samples( \
                const BackgroundSamples<C>&, const BackgroundSamples<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
