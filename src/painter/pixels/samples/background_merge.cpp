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
typename Color::DataType samples_weight_sum(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        return (a.full() ? a.weight_sum() : 0) + (b.full() ? b.weight_sum() : 0);
}

template <typename Color, typename Copy, typename Sum>
void merge_full_low(
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
void merge_full_high(
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

template <typename Color, typename Copy, typename Sum>
void merge_full(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b, const Copy copy, const Sum sum)
{
        merge_full_low(a, b, copy, sum);
        merge_full_high(a, b, copy, sum);
}

template <typename Color, typename Copy>
[[nodiscard]] std::array<std::make_signed_t<std::size_t>, 2> merge_partial_low(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b,
        const Copy copy)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        const std::size_t a_size = a.count();
        const std::size_t b_size = b.count();

        ASSERT(a_size > 0 && b_size > 0);
        ASSERT(a_size + b_size > COUNT);

        std::size_t a_i = 0;
        std::size_t b_i = 0;
        std::size_t i = 0;

        while (i < COUNT / 2 && a_i < a_size && b_i < b_size)
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

        while (i < COUNT / 2 && a_i < a_size)
        {
                copy(i++, a_i++, a);
        }

        while (i < COUNT / 2 && b_i < b_size)
        {
                copy(i++, b_i++, b);
        }

        return {static_cast<std::make_signed_t<std::size_t>>(a_i), static_cast<std::make_signed_t<std::size_t>>(b_i)};
}

template <typename Color, typename Copy>
[[nodiscard]] std::array<std::make_signed_t<std::size_t>, 2> merge_partial_high(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b,
        const std::make_signed_t<std::size_t> a_min,
        const std::make_signed_t<std::size_t> b_min,
        const Copy copy)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        const std::size_t a_size = a.count();
        const std::size_t b_size = b.count();

        ASSERT(a_size > 0 && b_size > 0);
        ASSERT(a_size + b_size > COUNT);

        std::make_signed_t<std::size_t> a_i = a_size - 1;
        std::make_signed_t<std::size_t> b_i = b_size - 1;
        std::size_t i = COUNT - 1;

        while (i >= COUNT / 2 && a_i >= a_min && b_i >= b_min)
        {
                if (a.weight(a_i) > b.weight(b_i))
                {
                        copy(i--, a_i--, a);
                }
                else
                {
                        copy(i--, b_i--, b);
                }
        }

        while (i >= COUNT / 2 && a_i >= a_min)
        {
                copy(i--, a_i--, a);
        }

        while (i >= COUNT / 2 && b_i >= b_min)
        {
                copy(i--, b_i--, b);
        }

        return {a_i, b_i};
}

template <typename Color, typename Copy, typename Sum>
void merge_partial(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b, const Copy copy, const Sum sum)
{
        const auto [a_low, b_low] = merge_partial_low(a, b, copy);
        const auto [a_high, b_high] = merge_partial_high(a, b, a_low, b_low, copy);

        for (auto i = a_low; i <= a_high; ++i)
        {
                sum(i, a);
        }

        for (auto i = b_low; i <= b_high; ++i)
        {
                sum(i, b);
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

        merge_full(
                a, b,
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

        ASSERT(!a.empty() && !b.empty());

        typename Color::DataType sum_weight = samples_weight_sum(a, b);
        std::array<typename Color::DataType, COUNT> weights;

        merge_partial(
                a, b,
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
