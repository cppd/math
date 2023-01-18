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

#include "background.h"

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
[[nodiscard]] T sum_weights(const std::vector<T>& weights, const std::size_t min_i, const std::size_t max_i)
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

template <typename Color>
BackgroundSamples<Color> merge_samples_empty(const BackgroundSamples<Color>& a)
{
        if (a.empty())
        {
                return BackgroundSamples<Color>{};
        }
        return a;
}

template <typename Color>
BackgroundSamples<Color> merge_samples_sum_only(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        ASSERT(a.sum_only() && !b.empty());

        if (b.sum_only())
        {
                if (a.sum_weight() < b.sum_weight())
                {
                        return {a.sum_weight(), b.sum_weight()};
                }
                return {b.sum_weight(), a.sum_weight()};
        }

        ASSERT(b.full());
        if (a.sum_weight() < b.min_weight())
        {
                return {b.sum_weight() + b.min_weight(), a.sum_weight(), b.max_weight()};
        }
        if (a.sum_weight() > b.max_weight())
        {
                return {b.sum_weight() + b.max_weight(), b.min_weight(), a.sum_weight()};
        }
        return {a.sum_weight() + b.sum_weight(), b.min_weight(), b.max_weight()};
}

template <typename Color>
BackgroundSamples<Color> merge_samples_full(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
{
        ASSERT(a.full() && b.full());

        typename Color::DataType sum_weight = a.sum_weight() + b.sum_weight();

        const BackgroundSamples<Color>* min = nullptr;
        const BackgroundSamples<Color>* max = nullptr;

        if (a.min_weight() < b.min_weight())
        {
                sum_weight += b.min_weight();
                min = &a;
        }
        else
        {
                sum_weight += a.min_weight();
                min = &b;
        }

        if (a.max_weight() > b.max_weight())
        {
                sum_weight += b.max_weight();
                max = &a;
        }
        else
        {
                sum_weight += a.max_weight();
                max = &b;
        }

        return {sum_weight, min->min_weight(), max->max_weight()};
}
}

template <typename T, typename Color>
std::optional<BackgroundSamples<Color>> make_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& color_weights)
{
        thread_local std::vector<typename Color::DataType> weights;

        const auto [min_i, max_i] = select_backgrounds_and_find_min_max(colors, color_weights, &weights);

        if (weights.empty())
        {
                return std::nullopt;
        }

        if (weights.size() == 1)
        {
                return BackgroundSamples<Color>{weights.front()};
        }

        if (weights.size() == 2)
        {
                return BackgroundSamples<Color>{weights[min_i], weights[max_i]};
        }

        return BackgroundSamples<Color>{sum_weights(weights, min_i, max_i), weights[min_i], weights[max_i]};
}

template <typename Color>
BackgroundSamples<Color> merge_background_samples(const BackgroundSamples<Color>& a, const BackgroundSamples<Color>& b)
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
        error("Failed to merge background samples");
}

#define TEMPLATE_T_C(T, C)                                                    \
        template std::optional<BackgroundSamples<C>> make_background_samples( \
                const std::vector<std::optional<C>>&, const std::vector<T>&);

#define TEMPLATE_C(C)                                           \
        template BackgroundSamples<C> merge_background_samples( \
                const BackgroundSamples<C>&, const BackgroundSamples<C>&);

TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)
TEMPLATE_INSTANTIATION_C(TEMPLATE_C)
}
