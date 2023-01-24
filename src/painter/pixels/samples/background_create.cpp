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
[[nodiscard]] BackgroundSamples<Color> create_samples(const std::vector<Sample<Color>>& samples)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();
        static_assert(COUNT % 2 == 0);

        std::array<typename Color::DataType, COUNT> weights;

        if (samples.size() <= COUNT)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        weights[i] = samples[i].weight;
                }
                return BackgroundSamples<Color>(weights, samples.size());
        }

        const std::size_t max_sum_i_s = samples.size() - COUNT / 2;

        std::size_t i_s = 0;
        std::size_t i_w = 0;
        typename Color::DataType sum = 0;

        while (i_w < COUNT / 2)
        {
                weights[i_w++] = samples[i_s++].weight;
        }
        while (i_s < max_sum_i_s)
        {
                sum += samples[i_s++].weight;
        }
        while (i_w < COUNT)
        {
                weights[i_w++] = samples[i_s++].weight;
        }
        ASSERT(i_w == weights.size());
        ASSERT(i_s == samples.size());

        return BackgroundSamples<Color>(sum, weights);
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights)
{
        static constexpr std::size_t COUNT = BackgroundSamples<Color>::size();

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

        static_assert(sizeof(Sample<Color>) == sizeof(typename Color::DataType));
        sort_samples<COUNT>(
                &samples,
                [](const Sample<Color> a, const Sample<Color> b)
                {
                        return a.weight < b.weight;
                },
                [](const Sample<Color> a, const Sample<Color> b)
                {
                        return a.weight > b.weight;
                });

        return create_samples(samples);
}

#define TEMPLATE_T_C(T, C)                                                      \
        template std::optional<BackgroundSamples<C>> create_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
}
