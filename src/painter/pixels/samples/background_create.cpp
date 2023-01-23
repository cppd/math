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

#include "background_create.h"

#include "sort.h"

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
        typename Color::DataType weight;
};

template <typename T, typename Color>
void select_backgrounds(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights,
        std::vector<Sample<Color>>* const samples)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        ASSERT(colors.size() == weights.size());

        samples->clear();

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (colors[i])
                {
                        continue;
                }

                const typename Color::DataType weight = weights[i];
                if (!(weight > 0))
                {
                        continue;
                }

                samples->push_back({.weight = weight});
        }
}

template <typename Color>
[[nodiscard]] typename Color::DataType sum_weight(
        const std::vector<Sample<Color>>& samples,
        const std::size_t min_i,
        const std::size_t max_i)
{
        typename Color::DataType res{0};

        if (samples.size() > 2)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                res += samples[i].weight;
                        }
                }
        }

        return res;
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        thread_local std::vector<Sample<Color>> samples;

        select_backgrounds(colors, weights, &samples);

        if (samples.empty())
        {
                return std::nullopt;
        }

        if (samples.size() == 1)
        {
                return BackgroundSamples<Color>({samples.front().weight}, 1);
        }

        thread_local std::vector<int> indices;

        indices.resize(samples.size());
        std::iota(indices.begin(), indices.end(), 0);

        const auto [min_i, max_i] = find_min_max(
                indices,
                [](const int a, const int b)
                {
                        return samples[a].weight < samples[b].weight;
                },
                [](const int a, const int b)
                {
                        return samples[a].weight > samples[b].weight;
                });

        const auto min = samples[min_i];
        const auto max = samples[max_i];

        if (samples.size() == 2)
        {
                return BackgroundSamples<Color>({min.weight, max.weight}, 2);
        }

        const auto weight_sum = sum_weight(samples, min_i, max_i);

        return BackgroundSamples<Color>(weight_sum, {min.weight, max.weight});
}

#define TEMPLATE_T_C(T, C)                                                      \
        template std::optional<BackgroundSamples<C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
