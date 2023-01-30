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
typename Color::DataType samples_weight_sum(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        return (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);
}

template <typename Color, typename Copy, typename Sum>
void merge_low_full(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b,
        const Copy copy,
        const Sum sum)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        std::size_t a_i = 0;
        std::size_t b_i = 0;

        for (std::size_t i = 0; i < COUNT / 2; ++i)
        {
                if (a.weight(a_i) < b.weight(b_i))
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
void merge_high_full(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b,
        const Copy copy,
        const Sum sum)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        std::size_t a_i = COUNT - 1;
        std::size_t b_i = COUNT - 1;

        for (std::size_t i = COUNT - 1; i >= COUNT / 2; --i)
        {
                if (a.weight(a_i) > b.weight(b_i))
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

template <typename Color, typename Copy>
void merge(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b, const Copy copy)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        const std::size_t a_size = a.count();
        const std::size_t b_size = b.count();

        ASSERT(a_size + b_size <= COUNT);

        std::size_t i = 0;
        std::size_t a_i = 0;
        std::size_t b_i = 0;

        while (a_i < a_size && b_i < b_size)
        {
                if (a.weight(a_i) < b.weight(b_i))
                {
                        copy(i++, a_i++, a);
                }
                else
                {
                        copy(i++, b_i++, b);
                }
        }

        while (a_i < a_size)
        {
                copy(i++, a_i++, a);
        }

        while (b_i < b_size)
        {
                copy(i++, b_i++, b);
        }
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_samples_full(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        ASSERT(a.count() == COUNT);
        ASSERT(b.count() == COUNT);

        typename Color::DataType sum_weight = samples_weight_sum(a, b);
        std::array<typename Color::DataType, COUNT> weights;

        const auto copy = [&](const std::size_t to, const std::size_t from, const BackgroundSamples<Color>& samples)
        {
                ASSERT(to < COUNT);
                weights[to] = samples.weight(from);
        };

        const auto sum = [&](const std::size_t index, const BackgroundSamples<Color>& samples)
        {
                sum_weight += samples.weight(index);
        };

        merge_low_full(a, b, copy, sum);
        merge_high_full(a, b, copy, sum);

        return BackgroundSamples<Color>(sum_weight, weights);
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_samples_partial(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        ASSERT(!a.empty() && !b.empty());

        const std::size_t a_b_count = a.count() + b.count();
        ASSERT(a_b_count <= COUNT);

        std::array<typename Color::DataType, COUNT> weights;

        merge(a, b,
              [&](const std::size_t to, const std::size_t from, const BackgroundSamples<Color>& samples)
              {
                      ASSERT(to < a_b_count);
                      weights[to] = samples.weight(from);
              });

        return BackgroundSamples<Color>(weights, a_b_count);
}
}

template <typename Color>
BackgroundSamples<Color> merge_background_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT == 2);

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
                return merge_samples_partial(a, b);
        }
        if (a_count == 1)
        {
                return merge_samples_one_sample(a, b);
        }
        if (b_count == 1)
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
