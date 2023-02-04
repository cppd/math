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

#include "com/merge.h"

#include <src/color/color.h>
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
[[nodiscard]] Merge<Color> merge(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        Merge<Color> res{
                .color = color_samples.full() ? color_samples.color_sum() : Color(0),
                .color_weight = color_samples.full() ? color_samples.weight_sum() : 0,
                .background_weight = background_samples.full() ? background_samples.weight_sum() : 0};

        com::merge_with_sum(
                color_samples, background_samples,
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        const auto a = color_samples.contribution(a_index);
                        const auto b = background_samples.weight(b_index) * background.contribution();
                        return a < b;
                },
                [&](const std::size_t a_index, const std::size_t b_index)
                {
                        const auto a = color_samples.contribution(a_index);
                        const auto b = background_samples.weight(b_index) * background.contribution();
                        return a > b;
                },
                [&](const std::size_t /*to*/, const std::size_t /*from*/, const auto& /*samples*/)
                {
                },
                [&](const std::size_t index, const auto& samples)
                {
                        using T = std::remove_cvref_t<decltype(samples)>;
                        static_assert(
                                std::is_same_v<T, ColorSamples<Color>> || std::is_same_v<T, BackgroundSamples<Color>>);
                        if constexpr (std::is_same_v<T, ColorSamples<Color>>)
                        {
                                res.color += samples.color(index);
                                res.color_weight += samples.weight(index);
                        }
                        else
                        {
                                res.background_weight += background_samples.weight(index);
                        }
                });

        return res;
}

template <typename Color>
[[nodiscard]] std::optional<Merge<Color>> merge_color_and_background(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        static_assert(ColorSamples<Color>::size() == BackgroundSamples<Color>::size());

        static constexpr std::size_t COUNT = ColorSamples<Color>::size();
        static_assert(COUNT >= 2);

        const std::size_t color_count = color_samples.count();
        const std::size_t background_count = background_samples.count();

        if (color_count + background_count > COUNT)
        {
                return merge(color_samples, background_samples, background);
        }

        if (color_count == 0)
        {
                return std::nullopt;
        }

        if (background_count == 0)
        {
                if (!color_samples.full())
                {
                        return std::nullopt;
                }

                return Merge<Color>{
                        .color = color_samples.color_sum(),
                        .color_weight = color_samples.weight_sum(),
                        .background_weight = 0};
        }

        return std::nullopt;
}
}

template <typename Color>
std::optional<Color> merge_color(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        const auto p = merge_color_and_background(color_samples, background_samples, background);

        if (!p)
        {
                return std::nullopt;
        }

        const auto sum = p->color_weight + p->background_weight;

        if (p->color_weight == sum || (p->color_weight / sum) == 1)
        {
                return p->color / sum;
        }

        if (p->background_weight == sum || (p->background_weight / sum) == 1)
        {
                return background.color();
        }

        return (p->color + p->background_weight * background.color()) / sum;
}

template <typename Color>
std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background)
{
        const auto p = merge_color_and_background(color_samples, background_samples, background);

        if (!p || p->color_weight == 0)
        {
                return std::nullopt;
        }

        const auto sum = p->color_weight + p->background_weight;

        return std::tuple(p->color / sum, p->color_weight / sum);
}

#define TEMPLATE_C(C)                                                                       \
        template std::optional<C> merge_color(                                              \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&); \
        template std::optional<std::tuple<C, typename C::DataType>> merge_color_alpha(      \
                const ColorSamples<C>&, const BackgroundSamples<C>&, const Background<C>&);

TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
