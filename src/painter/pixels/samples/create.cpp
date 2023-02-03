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

#include "create.h"

#include "../color_contribution.h"
#include "com/create.h"
#include "com/sort.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

#include <numeric>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
struct BackgroundSample final
{
        typename Color::DataType weight;
};

template <typename Color>
struct ColorSample final
{
        Color color;
        typename Color::DataType weight;
        typename Color::DataType contribution;
};

template <bool COLOR, typename Color, typename T, typename Select>
void select_samples(const std::vector<std::optional<Color>>& colors, const std::vector<T>& weights, const Select select)
{
        ASSERT(colors.size() == weights.size());

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (COLOR != colors[i].has_value())
                {
                        continue;
                }

                if (!(weights[i] > 0))
                {
                        continue;
                }

                select(i);
        }
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples_without_sum(const std::vector<BackgroundSample<Color>>& samples)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        std::array<typename Color::DataType, COUNT> weights;

        com::create_without_sum<BackgroundSamples<Color>>(
                samples.size(),
                [&](const std::size_t index)
                {
                        weights[index] = samples[index].weight;
                });

        return {weights, samples.size()};
}

template <typename Color>
[[nodiscard]] ColorSamples<Color> create_samples_without_sum(
        const std::vector<ColorSample<Color>>& samples,
        const std::vector<int>& indices)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        ASSERT(samples.size() == indices.size());

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        com::create_without_sum<ColorSamples<Color>>(
                samples.size(),
                [&](const std::size_t index)
                {
                        const ColorSample<Color>& sample = samples[indices[index]];
                        colors[index] = sample.color;
                        weights[index] = sample.weight;
                        contributions[index] = sample.contribution;
                });

        return {colors, weights, contributions, samples.size()};
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples_with_sum(const std::vector<BackgroundSample<Color>>& samples)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        typename Color::DataType sum{0};
        std::array<typename Color::DataType, COUNT> weights;

        com::create_with_sum<BackgroundSamples<Color>>(
                samples.size(),
                [&](const std::size_t to, const std::size_t from)
                {
                        weights[to] = samples[from].weight;
                },
                [&](const std::size_t index)
                {
                        sum += samples[index].weight;
                });

        return {sum, weights};
}

template <typename Color>
[[nodiscard]] ColorSamples<Color> create_samples_with_sum(
        const std::vector<ColorSample<Color>>& samples,
        const std::vector<int>& indices)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        ASSERT(samples.size() == indices.size());

        Color sum_color{0};
        typename Color::DataType sum_weight{0};
        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        com::create_with_sum<ColorSamples<Color>>(
                samples.size(),
                [&](const std::size_t to, const std::size_t from)
                {
                        const ColorSample<Color>& sample = samples[indices[from]];
                        colors[to] = sample.color;
                        weights[to] = sample.weight;
                        contributions[to] = sample.contribution;
                },
                [&](const std::size_t index)
                {
                        const ColorSample<Color>& sample = samples[indices[index]];
                        sum_color += sample.color;
                        sum_weight += sample.weight;
                });

        return {sum_color, colors, sum_weight, weights, contributions};
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples(std::vector<BackgroundSample<Color>>* const samples)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        ASSERT(!samples->empty());

        if (samples->size() == 1)
        {
                const auto& sample = samples->front();
                return {{sample.weight}, 1};
        }

        com::partial_sort<COUNT>(
                samples,
                [](const auto a, const auto b)
                {
                        return a.weight < b.weight;
                },
                [](const auto a, const auto b)
                {
                        return a.weight > b.weight;
                });

        if (samples->size() <= COUNT)
        {
                return create_samples_without_sum<Color>(*samples);
        }

        return create_samples_with_sum<Color>(*samples);
}

template <typename Color>
[[nodiscard]] ColorSamples<Color> create_samples(const std::vector<ColorSample<Color>>* const samples)
{
        static constexpr std::size_t COUNT = ColorSamples<Color>::size();

        ASSERT(!samples->empty());

        if (samples->size() == 1)
        {
                const auto& sample = samples->front();
                return {{sample.color}, {sample.weight}, {sample.contribution}, 1};
        }

        thread_local std::vector<int> indices;
        indices.resize(samples->size());
        std::iota(indices.begin(), indices.end(), 0);

        com::partial_sort<COUNT>(
                &indices,
                [&](const int a, const int b)
                {
                        return (*samples)[a].contribution < (*samples)[b].contribution;
                },
                [&](const int a, const int b)
                {
                        return (*samples)[a].contribution > (*samples)[b].contribution;
                });

        if (samples->size() <= COUNT)
        {
                return create_samples_without_sum(*samples, indices);
        }

        return create_samples_with_sum(*samples, indices);
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<BackgroundSample<Color>> samples;
        samples.clear();

        select_samples</*COLOR=*/false>(
                colors, weights,
                [&](const std::size_t index)
                {
                        const typename Color::DataType weight = weights[index];
                        samples.push_back({.weight = weight});
                });

        if (samples.empty())
        {
                return std::nullopt;
        }

        return create_samples(&samples);
}

template <typename T, typename Color>
std::optional<ColorSamples<Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<ColorSample<Color>> samples;
        samples.clear();

        select_samples</*COLOR=*/true>(
                colors, weights,
                [&](const std::size_t index)
                {
                        const Color& color = *colors[index];
                        const typename Color::DataType weight = weights[index];
                        samples.push_back(
                                {.color = weight * color,
                                 .weight = weight,
                                 .contribution = weight * sample_color_contribution(color)});
                });

        if (samples.empty())
        {
                return std::nullopt;
        }

        return create_samples(&samples);
}

#define TEMPLATE_T_C(T, C)                                                      \
        template std::optional<BackgroundSamples<C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);   \
        template std::optional<ColorSamples<C>> create_color_samples(           \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
