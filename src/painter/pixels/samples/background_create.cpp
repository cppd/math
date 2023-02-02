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

#include "com/create.h"
#include "com/sort.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <optional>
#include <vector>

namespace ns::painter::pixels::samples
{
namespace
{
template <typename T, typename Color>
void select_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights,
        std::vector<typename Color::DataType>* const samples)
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

                samples->push_back(weight);
        }
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples_without_sum(
        const std::vector<typename Color::DataType>& sample_weights)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        std::array<typename Color::DataType, COUNT> weights;

        com::create_without_sum<BackgroundSamples<Color>>(
                sample_weights.size(),
                [&](const std::size_t index)
                {
                        weights[index] = sample_weights[index];
                });

        return {weights, sample_weights.size()};
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples_with_sum(
        const std::vector<typename Color::DataType>& sample_weights)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        typename Color::DataType sum{0};
        std::array<typename Color::DataType, COUNT> weights;

        com::create_with_sum<BackgroundSamples<Color>>(
                sample_weights.size(),
                [&](const std::size_t to, const std::size_t from)
                {
                        weights[to] = sample_weights[from];
                },
                [&](const std::size_t index)
                {
                        sum += sample_weights[index];
                });

        return {sum, weights};
}

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> create_samples(std::vector<typename Color::DataType>&& sample_weights)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

        ASSERT(!sample_weights.empty());

        if (sample_weights.size() == 1)
        {
                return {{sample_weights.front()}, 1};
        }

        com::partial_sort<COUNT>(
                &sample_weights,
                [](const auto a, const auto b)
                {
                        return a < b;
                },
                [](const auto a, const auto b)
                {
                        return a > b;
                });

        if (sample_weights.size() <= COUNT)
        {
                return create_samples_without_sum<Color>(sample_weights);
        }

        return create_samples_with_sum<Color>(sample_weights);
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        thread_local std::vector<typename Color::DataType> sample_weights;

        select_background_samples(colors, weights, &sample_weights);

        if (sample_weights.empty())
        {
                return std::nullopt;
        }

        return create_samples<Color>(std::move(sample_weights));
}

#define TEMPLATE_T_C(T, C)                                                      \
        template std::optional<BackgroundSamples<C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
