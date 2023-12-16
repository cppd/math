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

#include "merge.h"

#include "background.h"
#include "color.h"

#include "com/info.h"
#include "com/merge.h"

#include <src/com/error.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <type_traits>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Samples, typename Sum>
[[nodiscard]] auto samples_sum(const Samples& a, const Samples& b, const Sum sum)
{
        if (a.full())
        {
                if (b.full())
                {
                        return sum(a) + sum(b);
                }
                return sum(a);
        }
        if (b.full())
        {
                return sum(b);
        }
        using T = std::remove_cvref_t<decltype(sum(a))>;
        return T{0};
}

template <typename Samples>
[[nodiscard]] decltype(auto) samples_weight_sum(const Samples& a, const Samples& b)
{
        return samples_sum(
                a, b,
                [](const Samples& s)
                {
                        return s.weight_sum();
                });
}

template <typename Samples>
[[nodiscard]] decltype(auto) samples_color_sum(const Samples& a, const Samples& b)
{
        return samples_sum(
                a, b,
                [](const Samples& s)
                {
                        return s.color_sum();
                });
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] BackgroundSamples<COUNT, Color> merge_with_sum(
        const BackgroundSamples<COUNT, Color>& a,
        const BackgroundSamples<COUNT, Color>& b)
{
        typename Color::DataType sum_weight = samples_weight_sum(a, b);
        std::array<typename Color::DataType, COUNT> weights;

        com::merge_with_sum(
                a, b,
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.weight(a_index) < b.weight(b_index);
                },
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.weight(a_index) > b.weight(b_index);
                },
                [&](const std::size_t to, const std::size_t from, const BackgroundSamples<COUNT, Color>& samples)
                {
                        ASSERT(to < COUNT);
                        weights[to] = samples.weight(from);
                },
                [&](const std::size_t index, const BackgroundSamples<COUNT, Color>& samples)
                {
                        sum_weight += samples.weight(index);
                });

        return {sum_weight, weights};
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] ColorSamples<COUNT, Color> merge_with_sum(
        const ColorSamples<COUNT, Color>& a,
        const ColorSamples<COUNT, Color>& b)
{
        Color sum_color = samples_color_sum(a, b);
        typename Color::DataType sum_weight = samples_weight_sum(a, b);

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        com::merge_with_sum(
                a, b,
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.contribution(a_index) < b.contribution(b_index);
                },
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.contribution(a_index) > b.contribution(b_index);
                },
                [&](const std::size_t to, const std::size_t from, const ColorSamples<COUNT, Color>& samples)
                {
                        ASSERT(to < COUNT);
                        colors[to] = samples.color(from);
                        weights[to] = samples.weight(from);
                        contributions[to] = samples.contribution(from);
                },
                [&](const std::size_t index, const ColorSamples<COUNT, Color>& samples)
                {
                        sum_color += samples.color(index);
                        sum_weight += samples.weight(index);
                });

        return {sum_color, colors, sum_weight, weights, contributions};
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] BackgroundSamples<COUNT, Color> merge(
        const BackgroundSamples<COUNT, Color>& a,
        const BackgroundSamples<COUNT, Color>& b)
{
        const std::size_t count = a.count() + b.count();

        std::array<typename Color::DataType, COUNT> weights;

        com::merge(
                a, b,
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.weight(a_index) < b.weight(b_index);
                },
                [&](const std::size_t to, const std::size_t from, const BackgroundSamples<COUNT, Color>& samples)
                {
                        ASSERT(to < count);
                        weights[to] = samples.weight(from);
                });

        return {weights, count};
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] ColorSamples<COUNT, Color> merge(const ColorSamples<COUNT, Color>& a, const ColorSamples<COUNT, Color>& b)
{
        const std::size_t count = a.count() + b.count();

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        com::merge(
                a, b,
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        return a.contribution(a_index) < b.contribution(b_index);
                },
                [&](const std::size_t to, const std::size_t from, const ColorSamples<COUNT, Color>& samples)
                {
                        ASSERT(to < count);
                        colors[to] = samples.color(from);
                        weights[to] = samples.weight(from);
                        contributions[to] = samples.contribution(from);
                });

        return {colors, weights, contributions, count};
}
}

template <typename Samples>
Samples merge_samples(const Samples& a, const Samples& b)
{
        static constexpr std::size_t COUNT = com::Info<Samples>::COUNT;
        static_assert(COUNT >= 2);

        const std::size_t a_count = a.count();
        const std::size_t b_count = b.count();

        if (a_count + b_count > COUNT)
        {
                return merge_with_sum(a, b);
        }

        if (a_count == 0)
        {
                return b;
        }

        if (b_count == 0)
        {
                return a;
        }

        return merge(a, b);
}

#define TEMPLATE_C_COUNT(C, COUNT)                                                           \
        template BackgroundSamples<(COUNT), C> merge_samples(                                \
                const BackgroundSamples<(COUNT), C>&, const BackgroundSamples<(COUNT), C>&); \
        template ColorSamples<(COUNT), C> merge_samples(                                     \
                const ColorSamples<(COUNT), C>&, const ColorSamples<(COUNT), C>&);

#define TEMPLATE_C(C)          \
        TEMPLATE_C_COUNT(C, 2) \
        TEMPLATE_C_COUNT(C, 4)

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
