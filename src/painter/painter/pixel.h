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

        Color m_color_sum{0};
        Color m_color_min{0};
        Color m_color_max{0};

        T m_color_weight_sum{0};
        T m_color_min_contribution{limits<T>::max()};
        T m_color_min_weight{0};
        T m_color_max_contribution{limits<T>::lowest()};
        T m_color_max_weight{0};

        T m_background_weight_sum{0};
        T m_background_min_weight{limits<T>::max()};
        T m_background_max_weight{limits<T>::lowest()};

        struct Data final
        {
                Color c;
                T c_w;
                T b_w;
        };

        Data color_data(const T& background_contribution) const
        {
                Data r{.c = m_color_sum, .c_w = m_color_weight_sum, .b_w = m_background_weight_sum};

                if (m_background_min_weight * background_contribution < m_color_min_contribution)
                {
                        r.c += m_color_min;
                        r.c_w += m_color_min_weight;
                }
                else
                {
                        r.b_w += m_background_min_weight;
                }

                if (m_background_max_weight * background_contribution > m_color_max_contribution)
                {
                        r.c += m_color_max;
                        r.c_w += m_color_max_weight;
                }
                else
                {
                        r.b_w += m_background_max_weight;
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
                m_color_sum += sum_color;
                m_color_weight_sum += sum_weight;

                if (min_contribution < m_color_min_contribution)
                {
                        m_color_sum += m_color_min;
                        m_color_weight_sum += m_color_min_weight;
                        m_color_min = min_color;
                        m_color_min_contribution = min_contribution;
                        m_color_min_weight = min_weight;
                }
                else
                {
                        m_color_sum += min_color;
                        m_color_weight_sum += min_weight;
                }

                if (max_contribution > m_color_max_contribution)
                {
                        m_color_sum += m_color_max;
                        m_color_weight_sum += m_color_max_weight;
                        m_color_max = max_color;
                        m_color_max_contribution = max_contribution;
                        m_color_max_weight = max_weight;
                }
                else
                {
                        m_color_sum += max_color;
                        m_color_weight_sum += max_weight;
                }
        }

        void merge_background(const T& sum_weight, const T& min_weight, const T& max_weight)
        {
                m_background_weight_sum += sum_weight;

                if (min_weight < m_background_min_weight)
                {
                        m_background_weight_sum += m_background_min_weight;
                        m_background_min_weight = min_weight;
                }
                else
                {
                        m_background_weight_sum += min_weight;
                }

                if (max_weight > m_background_max_weight)
                {
                        m_background_weight_sum += m_background_max_weight;
                        m_background_max_weight = max_weight;
                }
                else
                {
                        m_background_weight_sum += max_weight;
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
