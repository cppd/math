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
void select_color_samples(
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
[[nodiscard]] ColorSamples<Color> create_samples_without_sum(
        const std::vector<Sample<Color>>& samples,
        const std::vector<int>& indices)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        ASSERT(samples.size() == indices.size());
        ASSERT(!samples.empty() && samples.size() <= COUNT);

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
                const Sample<Color>& sample = samples[indices[i]];
                colors[i] = sample.color;
                weights[i] = sample.weight;
                contributions[i] = sample.contribution;
        }
        return {colors, weights, contributions, samples.size()};
}

template <typename Color>
[[nodiscard]] ColorSamples<Color> create_samples_with_sum(
        const std::vector<Sample<Color>>& samples,
        const std::vector<int>& indices)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();
        static_assert(COUNT % 2 == 0);

        ASSERT(samples.size() == indices.size());
        ASSERT(samples.size() > COUNT);

        Color sum_color{0};
        typename Color::DataType sum_weight{0};
        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        std::size_t sample_i = 0;

        for (std::size_t i = 0; i < COUNT / 2; ++i, ++sample_i)
        {
                const Sample<Color>& sample = samples[indices[sample_i]];
                colors[i] = sample.color;
                weights[i] = sample.weight;
                contributions[i] = sample.contribution;
        }

        const std::size_t sum_end = samples.size() - COUNT / 2;
        for (; sample_i < sum_end; ++sample_i)
        {
                const Sample<Color>& sample = samples[indices[sample_i]];
                sum_color += sample.color;
                sum_weight += sample.weight;
        }

        for (std::size_t i = COUNT / 2; i < COUNT; ++i, ++sample_i)
        {
                const Sample<Color>& sample = samples[indices[sample_i]];
                colors[i] = sample.color;
                weights[i] = sample.weight;
                contributions[i] = sample.contribution;
        }

        ASSERT(sample_i == samples.size());

        return {sum_color, colors, sum_weight, weights, contributions};
}

template <typename Color>
ColorSamples<Color> create_samples(std::vector<Sample<Color>>&& samples)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        ASSERT(!samples.empty());

        if (samples.size() == 1)
        {
                const auto& sample = samples.front();
                return {{sample.color}, {sample.weight}, {sample.contribution}, 1};
        }

        thread_local std::vector<int> indices;
        indices.resize(samples.size());
        std::iota(indices.begin(), indices.end(), 0);

        partial_sort<COUNT>(
                &indices,
                [&](const int a, const int b)
                {
                        return samples[a].contribution < samples[b].contribution;
                },
                [&](const int a, const int b)
                {
                        return samples[a].contribution > samples[b].contribution;
                });

        if (samples.size() <= COUNT)
        {
                return create_samples_without_sum(samples, indices);
        }

        return create_samples_with_sum(samples, indices);
}
}

template <typename T, typename Color>
std::optional<ColorSamples<Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<Sample<Color>> samples;

        select_color_samples(colors, weights, &samples);

        if (samples.empty())
        {
                return std::nullopt;
        }

        return create_samples(std::move(samples));
}

#define TEMPLATE_T_C(T, C)                                            \
        template std::optional<ColorSamples<C>> create_color_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
