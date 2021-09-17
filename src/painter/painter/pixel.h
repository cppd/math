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

#include <src/com/type/limit.h>

#include <optional>
#include <tuple>

namespace ns::painter
{
template <typename Color>
class Pixel final
{
        using T = typename Color::DataType;

        Color color_sum_{0};
        Color color_min_{0};
        Color color_max_{0};

        T color_weight_sum_{0};
        T color_min_contribution_{Limits<T>::max()};
        T color_min_weight_{0};
        T color_max_contribution_{Limits<T>::lowest()};
        T color_max_weight_{0};

        T background_weight_sum_{0};
        T background_min_weight_{Limits<T>::max()};
        T background_max_weight_{Limits<T>::lowest()};

        struct Data final
        {
                Color c;
                T c_w;
                T b_w;
        };

        Data color_data(const T& background_contribution) const
        {
                Data r{.c = color_sum_, .c_w = color_weight_sum_, .b_w = background_weight_sum_};

                if (background_min_weight_ * background_contribution < color_min_contribution_)
                {
                        r.c += color_min_;
                        r.c_w += color_min_weight_;
                }
                else
                {
                        r.b_w += background_min_weight_;
                }

                if (background_max_weight_ * background_contribution > color_max_contribution_)
                {
                        r.c += color_max_;
                        r.c_w += color_max_weight_;
                }
                else
                {
                        r.b_w += background_max_weight_;
                }

                return r;
        }

public:
        void merge_color(
                const Color& sum_color,
                const T& sum_weight,
                const Color& min_color,
                const T& min_contribution,
                const T& min_weight,
                const Color& max_color,
                const T& max_contribution,
                const T& max_weight)
        {
                color_sum_ += sum_color;
                color_weight_sum_ += sum_weight;

                if (min_contribution < color_min_contribution_)
                {
                        color_sum_ += color_min_;
                        color_weight_sum_ += color_min_weight_;
                        color_min_ = min_color;
                        color_min_contribution_ = min_contribution;
                        color_min_weight_ = min_weight;
                }
                else
                {
                        color_sum_ += min_color;
                        color_weight_sum_ += min_weight;
                }

                if (max_contribution > color_max_contribution_)
                {
                        color_sum_ += color_max_;
                        color_weight_sum_ += color_max_weight_;
                        color_max_ = max_color;
                        color_max_contribution_ = max_contribution;
                        color_max_weight_ = max_weight;
                }
                else
                {
                        color_sum_ += max_color;
                        color_weight_sum_ += max_weight;
                }
        }

        void merge_background(const T& sum_weight, const T& min_weight, const T& max_weight)
        {
                background_weight_sum_ += sum_weight;

                if (min_weight < background_min_weight_)
                {
                        background_weight_sum_ += background_min_weight_;
                        background_min_weight_ = min_weight;
                }
                else
                {
                        background_weight_sum_ += min_weight;
                }

                if (max_weight > background_max_weight_)
                {
                        background_weight_sum_ += background_max_weight_;
                        background_max_weight_ = max_weight;
                }
                else
                {
                        background_weight_sum_ += max_weight;
                }
        }

        std::optional<Color> color(const Color& background_color, const T& background_contribution) const
        {
                const Data data = color_data(background_contribution);

                const T sum = data.c_w + data.b_w;

                if (sum == data.b_w)
                {
                        return std::nullopt;
                }

                if (data.c_w == sum || (data.c_w / sum) == 1)
                {
                        return data.c / sum;
                }

                return (data.c + data.b_w * background_color) / sum;
        }

        std::optional<std::tuple<Color, T>> color_alpha(const T& background_contribution) const
        {
                const Data data = color_data(background_contribution);

                const T sum = data.c_w + data.b_w;

                if (sum == data.b_w)
                {
                        return std::nullopt;
                }

                if (data.c_w == sum || (data.c_w / sum) == 1)
                {
                        return std::make_tuple(data.c / sum, T(1));
                }

                return std::make_tuple(data.c / sum, data.c_w / sum);
        }
};
}
