/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "background.h"
#include "color.h"

#include "com/create.h"
#include "com/select.h"
#include "com/sort.h"

#include <src/com/error.h>
#include <src/painter/pixels/color_contribution.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <numeric>
#include <optional>
#include <vector>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename Color>
struct BackgroundSample final
{
        Color::DataType weight;
};

template <typename Color>
struct ColorSample final
{
        Color color;
        Color::DataType weight;
        Color::DataType contribution;
};

template <std::size_t COUNT, typename Color>
[[nodiscard]] BackgroundSamples<COUNT, Color> create_samples_without_sum(
        const std::vector<BackgroundSample<Color>>& samples)
{
        ASSERT(samples.size() <= COUNT);

        std::array<typename Color::DataType, COUNT> weights;

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
                weights[i] = samples[i].weight;
        }

        return {weights, samples.size()};
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] ColorSamples<COUNT, Color> create_samples_without_sum(
        const std::vector<ColorSample<Color>>& samples,
        const std::vector<int>& indices)
{
        ASSERT(samples.size() <= COUNT);
        ASSERT(samples.size() == indices.size());

        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
                const ColorSample<Color>& sample = samples[indices[i]];
                colors[i] = sample.color;
                weights[i] = sample.weight;
                contributions[i] = sample.contribution;
        }

        return {colors, weights, contributions, samples.size()};
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] BackgroundSamples<COUNT, Color> create_samples_with_sum(
        const std::vector<BackgroundSample<Color>>& samples)
{
        typename Color::DataType sum{0};
        std::array<typename Color::DataType, COUNT> weights;

        com::create_with_sum<BackgroundSamples<COUNT, Color>>(
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

template <std::size_t COUNT, typename Color>
[[nodiscard]] ColorSamples<COUNT, Color> create_samples_with_sum(
        const std::vector<ColorSample<Color>>& samples,
        const std::vector<int>& indices)
{
        ASSERT(samples.size() == indices.size());

        Color sum_color{0};
        typename Color::DataType sum_weight{0};
        std::array<Color, COUNT> colors;
        std::array<typename Color::DataType, COUNT> weights;
        std::array<typename Color::DataType, COUNT> contributions;

        com::create_with_sum<ColorSamples<COUNT, Color>>(
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

template <std::size_t COUNT, typename Color>
[[nodiscard]] BackgroundSamples<COUNT, Color> create_samples(std::vector<BackgroundSample<Color>>* const samples)
{
        ASSERT(!samples->empty());

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
                return create_samples_without_sum<COUNT>(*samples);
        }

        return create_samples_with_sum<COUNT>(*samples);
}

template <std::size_t COUNT, typename Color>
[[nodiscard]] ColorSamples<COUNT, Color> create_samples(const std::vector<ColorSample<Color>>* const samples)
{
        ASSERT(!samples->empty());

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
                return create_samples_without_sum<COUNT>(*samples, indices);
        }

        return create_samples_with_sum<COUNT>(*samples, indices);
}
}

template <std::size_t COUNT, typename T, typename Color>
std::optional<BackgroundSamples<COUNT, Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<BackgroundSample<Color>> samples;
        samples.clear();

        com::select_samples</*COLOR=*/false>(
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

        return create_samples<COUNT>(&samples);
}

template <std::size_t COUNT, typename T, typename Color>
std::optional<ColorSamples<COUNT, Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<ColorSample<Color>> samples;
        samples.clear();

        com::select_samples</*COLOR=*/true>(
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

        return create_samples<COUNT>(&samples);
}

#define TEMPLATE_T_C_COUNT(T, C, COUNT)                                                  \
        template std::optional<BackgroundSamples<(COUNT), C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);            \
        template std::optional<ColorSamples<(COUNT), C>> create_color_samples(           \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

#define TEMPLATE_T_C(T, C)          \
        TEMPLATE_T_C_COUNT(T, C, 2) \
        TEMPLATE_T_C_COUNT(T, C, 4)

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
