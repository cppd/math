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

#include "merge_color.h"

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
[[nodiscard]] Merge<Color> merge_color_one_sample_and_background_full(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(ColorSamples<Color>::size() == 2);
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.count() == 1);
        ASSERT(background_samples.count() == 2);

        const auto weight_sum = background_samples.full() ? background_samples.weight_sum() : 0;

        if (color_samples.contribution(0) < background_samples.weight(0) * background.contribution())
        {
                return {.color = Color{},
                        .color_weight = 0,
                        .background_weight = weight_sum + background_samples.weight(0)};
        }

        if (color_samples.contribution(0) > background_samples.weight(1) * background.contribution())
        {
                return {.color = Color{},
                        .color_weight = 0,
                        .background_weight = weight_sum + background_samples.weight(1)};
        }

        return {.color = color_samples.color(0),
                .color_weight = color_samples.weight(0),
                .background_weight = weight_sum};
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_full_and_background_one_sample(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(ColorSamples<Color>::size() == 2);
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.count() == 2);
        ASSERT(background_samples.count() == 1);

        const auto background_contribution = background_samples.weight(0) * background.contribution();

        const auto color_sum = color_samples.full() ? color_samples.color_sum() : Color(0);
        const auto weight_sum = color_samples.full() ? color_samples.weight_sum() : 0;

        if (background_contribution < color_samples.contribution(0))
        {
                return {.color = color_sum + color_samples.color(0),
                        .color_weight = weight_sum + color_samples.weight(0),
                        .background_weight = 0};
        }

        if (background_contribution > color_samples.contribution(1))
        {
                return {.color = color_sum + color_samples.color(1),
                        .color_weight = weight_sum + color_samples.weight(1),
                        .background_weight = 0};
        }

        return {.color = color_sum, .color_weight = weight_sum, .background_weight = background_samples.weight(0)};
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_full_and_background_full(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(ColorSamples<Color>::size() == 2);
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(color_samples.count() == 2);
        ASSERT(background_samples.count() == 2);

        Merge<Color> res{
                .color = color_samples.full() ? color_samples.color_sum() : Color(0),
                .color_weight = color_samples.full() ? color_samples.weight_sum() : 0,
                .background_weight = background_samples.full() ? background_samples.weight_sum() : 0};

        if (background_samples.weight(0) * background.contribution() < color_samples.contribution(0))
        {
                res.color += color_samples.color(0);
                res.color_weight += color_samples.weight(0);
        }
        else
        {
                res.background_weight += background_samples.weight(0);
        }

        if (background_samples.weight(1) * background.contribution() > color_samples.contribution(1))
        {
                res.color += color_samples.color(1);
                res.color_weight += color_samples.weight(1);
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
        static_assert(ColorSamples<Color>::size() == 2);
        static_assert(BackgroundSamples<Color>::size() == 2);

        ASSERT(!color_samples.empty());
        ASSERT(!background_samples.empty());

        if (color_samples.count() == 2)
        {
                if (background_samples.count() == 2)
                {
                        return merge_color_full_and_background_full(color_samples, background_samples, background);
                }
                return merge_color_full_and_background_one_sample(color_samples, background_samples, background);
        }

        if (background_samples.count() == 2)
        {
                return merge_color_one_sample_and_background_full(color_samples, background_samples, background);
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
        static_assert(ColorSamples<Color>::size() == 2);
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
                return color_samples.color_sum() / color_samples.weight_sum();
        }

        if (color_samples.count() == 1 && background_samples.count() == 1)
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
        static_assert(ColorSamples<Color>::size() == 2);
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
                return std::tuple(color_samples.color_sum() / color_samples.weight_sum(), ALPHA);
        }

        if (color_samples.count() == 1 && background_samples.count() == 1)
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
