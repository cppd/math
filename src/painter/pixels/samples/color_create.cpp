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

#include "color_create.h"

#include "sort.h"

#include "../color_contribution.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <numeric>
#include <optional>
#include <vector>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
struct Sample final
{
        Color color;
        typename Color::DataType weight;
        typename Color::DataType contribution;
};

template <typename T, typename Color>
void select_colors(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights,
        std::vector<Sample<Color>>* const samples)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        ASSERT(colors.size() == weights.size());

        samples->clear();

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                const auto& color = colors[i];
                if (!color)
                {
                        continue;
                }

                const typename Color::DataType weight = weights[i];
                if (!(weight > 0))
                {
                        continue;
                }

                samples->push_back(
                        {.color = weight * (*color),
                         .weight = weight,
                         .contribution = weight * sample_color_contribution(*color)});
        }
}

template <typename Color>
[[nodiscard]] std::tuple<Color, typename Color::DataType> sum_color_and_weight(
        const std::vector<Sample<Color>>& samples,
        const std::size_t min_i,
        const std::size_t max_i)
{
        std::tuple<Color, typename Color::DataType> res{0, 0};

        if (samples.size() > 2)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                std::get<0>(res) += samples[i].color;
                                std::get<1>(res) += samples[i].weight;
                        }
                }
        }

        return res;
}
}

template <typename T, typename Color>
std::optional<ColorSamples<Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        static_assert(ColorSamples<Color>::size() == 2);

        thread_local std::vector<Sample<Color>> samples;

        select_colors(colors, weights, &samples);

        if (samples.empty())
        {
                return std::nullopt;
        }

        if (samples.size() == 1)
        {
                const auto& sample = samples.front();
                return ColorSamples<Color>({sample.color}, {sample.weight}, {sample.contribution}, 1);
        }

        thread_local std::vector<int> indices;

        indices.resize(samples.size());
        std::iota(indices.begin(), indices.end(), 0);

        const auto [min_i, max_i] = find_min_max(
                indices,
                [](const int a, const int b)
                {
                        return samples[a].contribution < samples[b].contribution;
                },
                [](const int a, const int b)
                {
                        return samples[a].contribution > samples[b].contribution;
                });

        const auto& min = samples[min_i];
        const auto& max = samples[max_i];

        if (samples.size() == 2)
        {
                return ColorSamples<Color>(
                        {min.color, max.color}, {min.weight, max.weight}, {min.contribution, max.contribution}, 2);
        }

        const auto [color_sum, weight_sum] = sum_color_and_weight(samples, min_i, max_i);

        return ColorSamples<Color>(
                color_sum, {min.color, max.color}, weight_sum, {min.weight, max.weight},
                {min.contribution, max.contribution});
}

#define TEMPLATE_T_C(T, C)                                            \
        template std::optional<ColorSamples<C>> create_color_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
