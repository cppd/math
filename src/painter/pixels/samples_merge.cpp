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
[[nodiscard]] Merge<Color> merge_color_and_background(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const typename Color::DataType background_contribution)
{
        ASSERT(!color.empty());
        ASSERT(!background.empty());

        Merge<Color>
                res{.color = color.sum, .color_weight = color.sum_weight, .background_weight = background.sum_weight};

        const auto background_min_contribution = background.min_weight * background_contribution;
        const auto background_max_contribution = background.max_weight * background_contribution;

        if (background_min_contribution < color.min_contribution)
        {
                res.color += color.min;
                res.color_weight += color.min_weight;
        }
        else
        {
                res.background_weight += background.min_weight;
        }

        if (background_max_contribution > color.max_contribution)
        {
                res.color += color.max;
                res.color_weight += color.max_weight;
        }
        else
        {
                res.background_weight += background.max_weight;
        }

        return res;
}
}

template <typename Color>
void merge_color_samples(ColorSamples<Color>* const dst, const ColorSamples<Color>& samples)
{
        ASSERT(!samples.empty());

        dst->sum += samples.sum;
        dst->sum_weight += samples.sum_weight;

        if (samples.min_contribution < dst->min_contribution)
        {
                dst->sum += dst->min;
                dst->sum_weight += dst->min_weight;
                dst->min = samples.min;
                dst->min_contribution = samples.min_contribution;
                dst->min_weight = samples.min_weight;
        }
        else
        {
                dst->sum += samples.min;
                dst->sum_weight += samples.min_weight;
        }

        if (samples.max_contribution > dst->max_contribution)
        {
                dst->sum += dst->max;
                dst->sum_weight += dst->max_weight;
                dst->max = samples.max;
                dst->max_contribution = samples.max_contribution;
                dst->max_weight = samples.max_weight;
        }
        else
        {
                dst->sum += samples.max;
                dst->sum_weight += samples.max_weight;
        }
}

template <typename Color>
void merge_background_samples(BackgroundSamples<Color>* const dst, const BackgroundSamples<Color>& samples)
{
        ASSERT(!samples.empty());

        if (dst->empty())
        {
                *dst = samples;
                return;
        }

        dst->sum_weight += samples.sum_weight;

        if (samples.min_weight < dst->min_weight)
        {
                dst->sum_weight += dst->min_weight;
                dst->min_weight = samples.min_weight;
        }
        else
        {
                dst->sum_weight += samples.min_weight;
        }

        if (samples.max_weight > dst->max_weight)
        {
                dst->sum_weight += dst->max_weight;
                dst->max_weight = samples.max_weight;
        }
        else
        {
                dst->sum_weight += samples.max_weight;
        }
}

template <typename Color, typename ColorDataType>
std::optional<Color> merge_color(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const Color& background_color,
        const ColorDataType background_contribution)
{
        static_assert(std::is_same_v<ColorDataType, typename Color::DataType>);

        if (color.empty())
        {
                return std::nullopt;
        }

        if (background.empty())
        {
                return color.sum / color.sum_weight;
        }

        const Merge<Color> p = merge_color_and_background(color, background, background_contribution);

        const auto sum = p.color_weight + p.background_weight;

        if (p.color_weight == sum || (p.color_weight / sum) == 1)
        {
                return p.color / sum;
        }

        return (p.color + p.background_weight * background_color) / sum;
}

template <typename Color, typename ColorDataType>
std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const ColorDataType background_contribution)
{
        static_assert(std::is_same_v<ColorDataType, typename Color::DataType>);

        if (color.empty())
        {
                return std::nullopt;
        }

        if (background.empty())
        {
                return std::tuple(color.sum / color.sum_weight, static_cast<typename Color::DataType>(1));
        }

        const Merge<Color> p = merge_color_and_background(color, background, background_contribution);

        const auto sum = p.color_weight + p.background_weight;

        return std::tuple(p.color / sum, p.color_weight / sum);
}

#define TEMPLATE_C(C)                                                                                 \
        template void merge_color_samples(ColorSamples<C>*, const ColorSamples<C>&);                  \
        template void merge_background_samples(BackgroundSamples<C>*, const BackgroundSamples<C>&);   \
        template std::optional<C> merge_color(                                                        \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const C&, typename C::DataType); \
        template std::optional<std::tuple<C, typename C::DataType>> merge_color_alpha(                \
                const ColorSamples<C>&, const BackgroundSamples<C>&, typename C::DataType);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
