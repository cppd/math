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

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
struct Merge final
{
        Color color;
        typename Color::DataType color_weight;
        typename Color::DataType background_weight;
};

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_sum_only_and_background_full(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.sum_only());
        ASSERT(background_samples.count() == 2);

        const auto background_min_contribution = background_samples.weight(0) * background.contribution();
        const auto background_max_contribution = background_samples.weight(1) * background.contribution();
        const auto background_weight_sum = background_samples.full() ? background_samples.weight_sum() : 0;

        if (color_samples.sum_contribution() < background_min_contribution)
        {
                return {.color = Color{},
                        .color_weight = 0,
                        .background_weight = background_weight_sum + background_samples.weight(0)};
        }

        if (color_samples.sum_contribution() > background_max_contribution)
        {
                return {.color = Color{},
                        .color_weight = 0,
                        .background_weight = background_weight_sum + background_samples.weight(1)};
        }

        return {.color = color_samples.sum(),
                .color_weight = color_samples.sum_weight(),
                .background_weight = background_weight_sum};
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_full_and_background_one_sample(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.full());
        ASSERT(background_samples.count() == 1);

        const auto background_contribution = background_samples.weight(0) * background.contribution();

        if (background_contribution < color_samples.min_contribution())
        {
                return {.color = color_samples.sum() + color_samples.min(),
                        .color_weight = color_samples.sum_weight() + color_samples.min_weight(),
                        .background_weight = 0};
        }

        if (background_contribution > color_samples.max_contribution())
        {
                return {.color = color_samples.sum() + color_samples.max(),
                        .color_weight = color_samples.sum_weight() + color_samples.max_weight(),
                        .background_weight = 0};
        }

        return {.color = color_samples.sum(),
                .color_weight = color_samples.sum_weight(),
                .background_weight = background_samples.weight(0)};
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_full_and_background_full(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.full());
        ASSERT(background_samples.count() == 2);

        Merge<Color> res{
                .color = color_samples.sum(),
                .color_weight = color_samples.sum_weight(),
                .background_weight = background_samples.full() ? background_samples.weight_sum() : 0};

        const auto background_min_contribution = background_samples.weight(0) * background.contribution();
        const auto background_max_contribution = background_samples.weight(1) * background.contribution();

        if (background_min_contribution < color_samples.min_contribution())
        {
                res.color += color_samples.min();
                res.color_weight += color_samples.min_weight();
        }
        else
        {
                res.background_weight += background_samples.weight(0);
        }

        if (background_max_contribution > color_samples.max_contribution())
        {
                res.color += color_samples.max();
                res.color_weight += color_samples.max_weight();
        }
        else
        {
                res.background_weight += background_samples.weight(1);
        }

        return res;
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_and_background(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(!color_samples.empty());
        ASSERT(!background_samples.empty());

        if (color_samples.full())
        {
                if (background_samples.count() == 2)
                {
                        return merge_color_full_and_background_full(color_samples, background_samples, background);
                }
                return merge_color_full_and_background_one_sample(color_samples, background_samples, background);
        }

        if (background_samples.count() == 2)
        {
                return merge_color_sum_only_and_background_full(color_samples, background_samples, background);
        }

        error("Failed to merge color and background samples");
}
}

template <typename Color>
std::optional<Color> merge_color(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        if (color_samples.empty())
        {
                return std::nullopt;
        }

        if (background_samples.empty())
        {
                if (!color_samples.full())
                {
                        return std::nullopt;
                }
                return color_samples.sum() / color_samples.sum_weight();
        }

        if (color_samples.sum_only() && background_samples.count() == 1)
        {
                return std::nullopt;
        }

        const Merge<Color> p = merge_color_and_background(color_samples, background_samples, background);

        const auto sum = p.color_weight + p.background_weight;

        if (p.color_weight == sum || (p.color_weight / sum) == 1)
        {
                return p.color / sum;
        }

        if (p.background_weight == sum || (p.background_weight / sum) == 1)
        {
                return background.color();
        }

        return (p.color + p.background_weight * background.color()) / sum;
}

template <typename Color>
std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        if (color_samples.empty())
        {
                return std::nullopt;
        }

        if (background_samples.empty())
        {
                if (!color_samples.full())
                {
                        return std::nullopt;
                }
                constexpr typename Color::DataType ALPHA{1};
                return std::tuple(color_samples.sum() / color_samples.sum_weight(), ALPHA);
        }

        if (color_samples.sum_only() && background_samples.count() == 1)
        {
                return std::nullopt;
        }

        const Merge<Color> p = merge_color_and_background(color_samples, background_samples, background);

        if (p.color_weight == 0)
        {
                return std::nullopt;
        }

        const auto sum = p.color_weight + p.background_weight;

        return std::tuple(p.color / sum, p.color_weight / sum);
}

#define TEMPLATE_C(C)                                                                       \
        template std::optional<C> merge_color(                                              \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&); \
        template std::optional<std::tuple<C, typename C::DataType>> merge_color_alpha(      \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
