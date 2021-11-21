/*
Copyright (C) 2017-2021 Topological Manifold

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

#pragma once

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <optional>
#include <vector>

namespace ns::painter
{
template <typename Color>
auto pixel_samples_color_contribution(const Color& color)
{
        return color.luminance();
}

template <typename Color>
struct ColorSamples final
{
        Color sum;
        Color min;
        Color max;

        typename Color::DataType sum_weight;
        typename Color::DataType min_contribution;
        typename Color::DataType min_weight;
        typename Color::DataType max_contribution;
        typename Color::DataType max_weight;

        ColorSamples()
        {
        }

        void init()
        {
                sum = Color(0);
                min = Color(0);
                max = Color(0);

                sum_weight = 0;
                min_contribution = Limits<decltype(min_contribution)>::max();
                min_weight = 0;
                max_contribution = Limits<decltype(max_contribution)>::lowest();
                max_weight = 0;
        }

        void merge(const ColorSamples<Color>& samples)
        {
                sum += samples.sum;
                sum_weight += samples.sum_weight;

                if (samples.min_contribution < min_contribution)
                {
                        sum += min;
                        sum_weight += min_weight;
                        min = samples.min;
                        min_contribution = samples.min_contribution;
                        min_weight = samples.min_weight;
                }
                else
                {
                        sum += samples.min;
                        sum_weight += samples.min_weight;
                }

                if (samples.max_contribution > max_contribution)
                {
                        sum += max;
                        sum_weight += max_weight;
                        max = samples.max;
                        max_contribution = samples.max_contribution;
                        max_weight = samples.max_weight;
                }
                else
                {
                        sum += samples.max;
                        sum_weight += samples.max_weight;
                }
        }
};

template <typename Color>
struct BackgroundSamples final
{
        typename Color::DataType sum_weight;
        typename Color::DataType min_weight;
        typename Color::DataType max_weight;

        BackgroundSamples()
        {
        }

        void init()
        {
                sum_weight = 0;
                min_weight = Limits<decltype(min_weight)>::max();
                max_weight = Limits<decltype(max_weight)>::lowest();
        }

        void merge(const BackgroundSamples<Color>& samples)
        {
                sum_weight += samples.sum_weight;

                if (samples.min_weight < min_weight)
                {
                        sum_weight += min_weight;
                        min_weight = samples.min_weight;
                }
                else
                {
                        sum_weight += samples.min_weight;
                }

                if (samples.max_weight > max_weight)
                {
                        sum_weight += max_weight;
                        max_weight = samples.max_weight;
                }
                else
                {
                        sum_weight += samples.max_weight;
                }
        }
};

template <typename Color>
struct PixelSamples final
{
        Color color;
        typename Color::DataType color_weight;
        typename Color::DataType background_weight;
};

template <typename Color>
PixelSamples<Color> merge_color_and_background(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const typename Color::DataType& background_contribution)
{
        using T = typename Color::DataType;

        PixelSamples<Color> res{
                .color = color.sum,
                .color_weight = color.sum_weight,
                .background_weight = background.sum_weight};

        const T background_min_contribution = background.min_weight * background_contribution;
        const T background_max_contribution = background.max_weight * background_contribution;

        if (background_min_contribution < color.min_contribution)
        {
                res.color += color.min;
                res.color_weight += color.min_weight;
        }
        else
        {
                res.background_weight += background.min_weight;
        }

        if (background_max_contribution > color.max_contribution)
        {
                res.color += color.max;
                res.color_weight += color.max_weight;
        }
        else
        {
                res.background_weight += background.max_weight;
        }

        return res;
}

template <typename Color, typename Weight>
std::optional<ColorSamples<Color>> make_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<Weight>& color_weights)
{
        using T = typename Color::DataType;

        ASSERT(colors.size() == color_weights.size());

        thread_local std::vector<Color> samples;
        thread_local std::vector<T> contributions;
        thread_local std::vector<T> weights;

        samples.clear();
        contributions.clear();
        weights.clear();

        T min = Limits<T>::max();
        T max = Limits<T>::lowest();
        std::size_t min_i = Limits<std::size_t>::max();
        std::size_t max_i = Limits<std::size_t>::max();

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (!colors[i])
                {
                        continue;
                }

                const T weight = color_weights[i];

                if (!(weight > 0))
                {
                        continue;
                }

                samples.push_back(weight * (*colors[i]));
                contributions.push_back(weight * pixel_samples_color_contribution(*colors[i]));
                weights.push_back(weight);

                if (contributions.back() < min)
                {
                        min = contributions.back();
                        min_i = samples.size() - 1;
                }

                if (contributions.back() > max)
                {
                        max = contributions.back();
                        max_i = samples.size() - 1;
                }
        }

        if (samples.empty())
        {
                return {};
        }

        ASSERT(min_i < samples.size());
        ASSERT(max_i < samples.size());

        std::optional<ColorSamples<Color>> res(std::in_place);

        res->sum = Color(0);
        res->sum_weight = 0;

        res->min = samples[min_i];
        res->min_contribution = contributions[min_i];
        res->min_weight = weights[min_i];

        res->max = samples[max_i];
        res->max_contribution = contributions[max_i];
        res->max_weight = weights[max_i];

        if (samples.size() > 2)
        {
                for (std::size_t i = 0; i < samples.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                res->sum += samples[i];
                                res->sum_weight += weights[i];
                        }
                }
        }

        return res;
}

template <typename Color, typename Weight>
std::optional<BackgroundSamples<Color>> make_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<Weight>& color_weights)
{
        using T = typename Color::DataType;

        ASSERT(colors.size() == color_weights.size());

        thread_local std::vector<T> weights;

        weights.clear();

        T min = Limits<T>::max();
        T max = Limits<T>::lowest();
        std::size_t min_i = Limits<std::size_t>::max();
        std::size_t max_i = Limits<std::size_t>::max();

        for (std::size_t i = 0; i < colors.size(); ++i)
        {
                if (colors[i])
                {
                        continue;
                }

                const T weight = color_weights[i];

                if (!(weight > 0))
                {
                        continue;
                }

                weights.push_back(weight);

                if (weight < min)
                {
                        min = weight;
                        min_i = weights.size() - 1;
                }

                if (weight > max)
                {
                        max = weight;
                        max_i = weights.size() - 1;
                }
        }

        if (weights.empty())
        {
                return std::nullopt;
        }

        ASSERT(min_i < weights.size());
        ASSERT(max_i < weights.size());

        std::optional<BackgroundSamples<Color>> res(std::in_place);

        res->sum_weight = 0;
        res->min_weight = weights[min_i];
        res->max_weight = weights[max_i];

        if (weights.size() > 2)
        {
                for (std::size_t i = 0; i < weights.size(); ++i)
                {
                        if (i != min_i && i != max_i)
                        {
                                res->sum_weight += weights[i];
                        }
                }
        }

        return res;
}
}
