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
[[nodiscard]] ColorSamples<Color> create_samples(
        const std::vector<Sample<Color>>& samples,
        const std::vector<int>& indices)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();
        static_assert(COUNT % 2 == 0);

        ASSERT(samples.size() == indices.size());

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        if (samples.size() <= COUNT)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        const Sample<Color>& sample = samples[indices[i]];
                        colors[i] = sample.color;
                        weights[i] = sample.weight;
                        contributions[i] = sample.contribution;
                }
                return ColorSamples<Color>(colors, weights, contributions, samples.size());
        }

        const std::size_t max_sum_i_s = samples.size() - COUNT / 2;

        std::size_t i_r{0};
        std::size_t i_s{0};
        Color sum_color{0};
        typename Color::DataType sum_weight{0};

        while (i_r < COUNT / 2)
        {
                const Sample<Color>& sample = samples[indices[i_s++]];
                colors[i_r] = sample.color;
                weights[i_r] = sample.weight;
                contributions[i_r] = sample.contribution;
                ++i_r;
        }
        while (i_s < max_sum_i_s)
        {
                const Sample<Color>& sample = samples[indices[i_s++]];
                sum_color += sample.color;
                sum_weight += sample.weight;
        }
        while (i_r < COUNT)
        {
                const Sample<Color>& sample = samples[indices[i_s++]];
                colors[i_r] = sample.color;
                weights[i_r] = sample.weight;
                contributions[i_r] = sample.contribution;
                ++i_r;
        }
        ASSERT(i_r == COUNT);
        ASSERT(i_s == samples.size());

        return ColorSamples<Color>(sum_color, colors, sum_weight, weights, contributions);
}
}

template <typename T, typename Color>
std::optional<ColorSamples<Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

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

        sort_samples<COUNT>(
                &indices,
                [](const int a, const int b)
                {
                        return samples[a].contribution < samples[b].contribution;
                },
                [](const int a, const int b)
                {
                        return samples[a].contribution > samples[b].contribution;
                });

        return create_samples(samples, indices);
}

#define TEMPLATE_T_C(T, C)                                            \
        template std::optional<ColorSamples<C>> create_color_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
