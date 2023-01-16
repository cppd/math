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

#include "samples_color.h"

#include "color_contribution.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter::pixels
{
namespace
{
template <typename T, typename Color>
[[nodiscard]] std::tuple<std::size_t, std::size_t> select_colors_and_find_min_max(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights,
        std::vector<Color>* const samples,
        std::vector<typename Color::DataType>* const contributions,
        std::vector<typename Color::DataType>* const weights)
{
        static_assert(std::is_floating_point_v<typename Color::DataType>);

        ASSERT(colors.size() == color_weights.size());

        samples->clear();
        contributions->clear();
        weights->clear();

        typename Color::DataType min = Limits<decltype(min)>::infinity();
        typename Color::DataType max = -Limits<decltype(max)>::infinity();
        std::size_t min_i = -1;
        std::size_t max_i = -1;

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                const auto& color = colors[i];
                if (!color)
                {
                        continue;
                }

                const typename Color::DataType weight = color_weights[i];
                if (!(weight > 0))
                {
                        continue;
                }

                samples->push_back(weight * (*color));
                contributions->push_back(weight * sample_color_contribution(*color));
                weights->push_back(weight);

                if (contributions->back() < min)
                {
                        min = contributions->back();
                        min_i = samples->size() - 1;
                }

                if (contributions->back() > max)
                {
                        max = contributions->back();
                        max_i = samples->size() - 1;
                }
        }

        if (samples->empty() || samples->size() == 1)
        {
                return {};
        }

        ASSERT(min_i < samples->size());
        ASSERT(max_i < samples->size());
        if (min_i == max_i)
        {
                // all contributions are equal
                return {0, 1};
        }
        return {min_i, max_i};
}

template <typename Color>
[[nodiscard]] std::tuple<Color, typename Color::DataType> sum_samples_and_weights(
        const std::vector<Color>& samples,
        const std::vector<typename Color::DataType>& weights,
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
                                std::get<0>(res) += samples[i];
                                std::get<1>(res) += weights[i];
                        }
                }
        }

        return res;
}

template <typename Color>
ColorSamples<Color> merge_samples_empty(const ColorSamples<Color>& a)
{
        if (a.empty())
        {
                return ColorSamples<Color>{};
        }
        return a;
}

template <typename Color>
ColorSamples<Color> merge_samples_sum_only(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        ASSERT(a.sum_only() && !b.empty());

        if (b.sum_only())
        {
                const auto create = [](const ColorSamples<Color>& min, const ColorSamples<Color>& max)
                {
                        return ColorSamples{
                                min.sum(),
                                max.sum(),
                                min.sum_weight(),
                                max.sum_weight(),
                                min.sum_contribution(),
                                max.sum_contribution()};
                };
                if (a.sum_contribution() < b.sum_contribution())
                {
                        return create(a, b);
                }
                return create(b, a);
        }

        ASSERT(b.full());
        if (a.sum_contribution() < b.min_contribution())
        {
                return {b.sum() + b.min(),
                        a.sum(),
                        b.max(),
                        b.sum_weight() + b.min_weight(),
                        a.sum_weight(),
                        b.max_weight(),
                        a.sum_contribution(),
                        b.max_contribution()};
        }
        if (a.sum_contribution() > b.max_contribution())
        {
                return {b.sum() + b.max(),
                        b.min(),
                        a.sum(),
                        b.sum_weight() + b.max_weight(),
                        b.min_weight(),
                        a.sum_weight(),
                        b.min_contribution(),
                        a.sum_contribution()};
        }
        return {a.sum() + b.sum(),
                b.min(),
                b.max(),
                a.sum_weight() + b.sum_weight(),
                b.min_weight(),
                b.max_weight(),
                b.min_contribution(),
                b.max_contribution()};
}

template <typename Color>
ColorSamples<Color> merge_samples_full(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        ASSERT(a.full() && b.full());

        Color sum = a.sum() + b.sum();
        typename Color::DataType sum_weight = a.sum_weight() + b.sum_weight();

        const ColorSamples<Color>* min = nullptr;
        const ColorSamples<Color>* max = nullptr;

        if (a.min_contribution() < b.min_contribution())
        {
                sum += b.min();
                sum_weight += b.min_weight();
                min = &a;
        }
        else
        {
                sum += a.min();
                sum_weight += a.min_weight();
                min = &b;
        }

        if (a.max_contribution() > b.max_contribution())
        {
                sum += b.max();
                sum_weight += b.max_weight();
                max = &a;
        }
        else
        {
                sum += a.max();
                sum_weight += a.max_weight();
                max = &b;
        }

        return {sum,
                min->min(),
                max->max(),
                sum_weight,
                min->min_weight(),
                max->max_weight(),
                min->min_contribution(),
                max->max_contribution()};
}
}

template <typename T, typename Color>
std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights)
{
        thread_local std::vector<Color> samples;
        thread_local std::vector<typename Color::DataType> contributions;
        thread_local std::vector<typename Color::DataType> weights;

        const auto [min_i, max_i] =
                select_colors_and_find_min_max(colors, color_weights, &samples, &contributions, &weights);

        ASSERT(samples.size() == contributions.size());
        ASSERT(contributions.size() == weights.size());

        if (samples.empty())
        {
                return std::nullopt;
        }

        if (samples.size() == 1)
        {
                return ColorSamples<Color>(samples.front(), weights.front(), contributions.front());
        }

        if (samples.size() == 2)
        {
                return ColorSamples<Color>(
                        samples[min_i], samples[max_i], weights[min_i], weights[max_i], contributions[min_i],
                        contributions[max_i]);
        }

        const auto [sum_samples, sum_weights] = sum_samples_and_weights(samples, weights, min_i, max_i);

        return ColorSamples<Color>(
                sum_samples, samples[min_i], samples[max_i], sum_weights, weights[min_i], weights[max_i],
                contributions[min_i], contributions[max_i]);
}

template <typename Color>
ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b)
{
        if (a.full() && b.full())
        {
                return merge_samples_full(a, b);
        }
        if (a.empty())
        {
                return merge_samples_empty(b);
        }
        if (b.empty())
        {
                return merge_samples_empty(a);
        }
        if (a.sum_only())
        {
                return merge_samples_sum_only(a, b);
        }
        if (b.sum_only())
        {
                return merge_samples_sum_only(b, a);
        }
        error("Failed to merge color samples");
}

#define TEMPLATE_T_C(T, C)                                          \
        template std::optional<ColorSamples<C>> make_color_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

#define TEMPLATE_C(C) template ColorSamples<C> merge_color_samples(const ColorSamples<C>&, const ColorSamples<C>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
