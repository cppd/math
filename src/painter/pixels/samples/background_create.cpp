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
template <typename T, typename Color>
[[nodiscard]] std::tuple<std::size_t, std::size_t> select_backgrounds_and_find_min_max(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights,
        std::vector<typename Color::DataType>* const weights)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        ASSERT(colors.size() == color_weights.size());

        weights->clear();

        typename Color::DataType min = Limits<decltype(min)>::infinity();
        typename Color::DataType max = -Limits<decltype(max)>::infinity();
        std::size_t min_i = -1;
        std::size_t max_i = -1;

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (colors[i])
                {
                        continue;
                }

                const typename Color::DataType weight = color_weights[i];
                if (!(weight > 0))
                {
                        continue;
                }

                weights->push_back(weight);

                if (weight < min)
                {
                        min = weight;
                        min_i = weights->size() - 1;
                }

                if (weight > max)
                {
                        max = weight;
                        max_i = weights->size() - 1;
                }
        }

        if (weights->empty() || weights->size() == 1)
        {
                return {};
        }

        ASSERT(min_i < weights->size());
        ASSERT(max_i < weights->size());
        if (min_i == max_i)
        {
                // all elements are equal
                return {0, 1};
        }
        return {min_i, max_i};
}

template <typename T>
[[nodiscard]] T weight_sum(const std::vector<T>& weights, const std::size_t min_i, const std::size_t max_i)
{
        T res{0};

        if (weights.size() > 2)
        {
                for (std::size_t i = 0; i < weights.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                res += weights[i];
                        }
                }
        }

        return res;
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights)
{
        static_assert(BackgroundSamples<Color>::size() == 2);

        thread_local std::vector<typename Color::DataType> weights;

        const auto [min_i, max_i] = select_backgrounds_and_find_min_max(colors, color_weights, &weights);

        if (weights.empty())
        {
                return std::nullopt;
        }

        if (weights.size() == 1)
        {
                return BackgroundSamples<Color>({weights.front()}, 1);
        }

        if (weights.size() == 2)
        {
                return BackgroundSamples<Color>({weights[min_i], weights[max_i]}, 2);
        }

        return BackgroundSamples<Color>(weight_sum(weights, min_i, max_i), {weights[min_i], weights[max_i]});
}

#define TEMPLATE_T_C(T, C)                                                      \
        template std::optional<BackgroundSamples<C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
