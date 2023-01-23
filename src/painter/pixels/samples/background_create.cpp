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

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <optional>
#include <tuple>
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
[[nodiscard]] std::tuple<std::size_t, std::size_t> find_min_max(const std::vector<Sample<Color>>& samples)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        if (samples.empty() || samples.size() == 1)
        {
                return {};
        }

        typename Color::DataType min = Limits<decltype(min)>::infinity();
        typename Color::DataType max = -Limits<decltype(max)>::infinity();
        std::size_t min_i = -1;
        std::size_t max_i = -1;

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
                const auto weight = samples[i].weight;

                if (weight < min)
                {
                        min = weight;
                        min_i = i;
                }

                if (weight > max)
                {
                        max = weight;
                        max_i = i;
                }
        }

        ASSERT(min_i < samples.size());
        ASSERT(max_i < samples.size());
        if (min_i == max_i)
        {
                // all elements are equal
                return {0, 1};
        }
        return {min_i, max_i};
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

        const auto [min_i, max_i] = find_min_max(samples);

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
