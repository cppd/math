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

#include "samples_merge.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

namespace ns::painter::pixels
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
[[nodiscard]] Merge<Color> merge_color_and_background_sum_only(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        ASSERT(background_samples.sum_only());

        const auto background_contribution = background_samples.sum_weight() * background.contribution();

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
                .background_weight = background_samples.sum_weight()};
}

template <typename Color>
[[nodiscard]] Merge<Color> merge_color_and_background(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        ASSERT(!color_samples.empty());
        ASSERT(!background_samples.empty());

        if (background_samples.sum_only())
        {
                return merge_color_and_background_sum_only(color_samples, background_samples, background);
        }

        Merge<Color> res{
                .color = color_samples.sum(),
                .color_weight = color_samples.sum_weight(),
                .background_weight = background_samples.sum_weight()};

        const auto background_min_contribution = background_samples.min_weight() * background.contribution();
        const auto background_max_contribution = background_samples.max_weight() * background.contribution();

        if (background_min_contribution < color_samples.min_contribution())
        {
                res.color += color_samples.min();
                res.color_weight += color_samples.min_weight();
        }
        else
        {
                res.background_weight += background_samples.min_weight();
        }

        if (background_max_contribution > color_samples.max_contribution())
        {
                res.color += color_samples.max();
                res.color_weight += color_samples.max_weight();
        }
        else
        {
                res.background_weight += background_samples.max_weight();
        }

        return res;
}
}

template <typename Color>
ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        if (a.empty())
        {
                if (b.empty())
                {
                        return {};
                }
                return b;
        }

        if (b.empty())
        {
                if (a.empty())
                {
                        return {};
                }
                return a;
        }

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

template <typename Color>
std::optional<Color> merge_color(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        if (color_samples.empty())
        {
                return std::nullopt;
        }

        if (background_samples.empty())
        {
                return color_samples.sum() / color_samples.sum_weight();
        }

        const Merge<Color> p = merge_color_and_background(color_samples, background_samples, background);

        const auto sum = p.color_weight + p.background_weight;

        if (p.color_weight == sum || (p.color_weight / sum) == 1)
        {
                return p.color / sum;
        }

        return (p.color + p.background_weight * background.color()) / sum;
}

template <typename Color>
std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        if (color_samples.empty())
        {
                return std::nullopt;
        }

        if (background_samples.empty())
        {
                constexpr typename Color::DataType ALPHA{1};
                return std::tuple(color_samples.sum() / color_samples.sum_weight(), ALPHA);
        }

        const Merge<Color> p = merge_color_and_background(color_samples, background_samples, background);

        const auto sum = p.color_weight + p.background_weight;

        return std::tuple(p.color / sum, p.color_weight / sum);
}

#define TEMPLATE_C(C)                                                                                 \
        template ColorSamples<C> merge_color_samples(const ColorSamples<C>&, const ColorSamples<C>&); \
        template std::optional<C> merge_color(                                                        \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&);           \
        template std::optional<std::tuple<C, typename C::DataType>> merge_color_alpha(                \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
