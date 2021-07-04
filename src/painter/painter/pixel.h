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
        using DataType = typename Color::DataType;

        Color m_color_sum{0};
        DataType m_color_weight_sum{0};

        Color m_color_min{0};
        DataType m_color_min_contribution{limits<DataType>::max()};
        DataType m_color_min_weight{0};

        Color m_color_max{0};
        DataType m_color_max_contribution{limits<DataType>::lowest()};
        DataType m_color_max_weight{0};

        DataType m_background_weight_sum{0};
        DataType m_background_min_weight{limits<DataType>::max()};
        DataType m_background_max_weight{limits<DataType>::lowest()};

        struct Data final
        {
                Color c;
                DataType c_w;
                DataType b_w;
        };

        Data color_data(DataType background_contribution) const
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
                DataType sum_weight,
                const Color& min_color,
                DataType min_contribution,
                DataType min_weight,
                const Color& max_color,
                DataType max_contribution,
                DataType max_weight)
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

        void merge_background(DataType sum_weight, DataType min_weight, DataType max_weight)
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

        std::optional<Color> color(const Color& background_color, DataType background_contribution) const
        {
                const Data data = color_data(background_contribution);

                if (data.c_w == 0)
                {
                        return std::nullopt;
                }

                if (data.b_w == 0)
                {
                        return data.c / data.c_w;
                }

                return (data.c + data.b_w * background_color) / (data.c_w + data.b_w);
        }

        std::optional<std::tuple<Color, DataType>> color_alpha(DataType background_contribution) const
        {
                const Data data = color_data(background_contribution);

                if (data.c_w == 0)
                {
                        return std::nullopt;
                }

                if (data.b_w == 0)
                {
                        return std::make_tuple(data.c / data.c_w, DataType(1));
                }

                const DataType sum = data.c_w + data.b_w;
                return std::make_tuple(data.c / sum, data.c_w / sum);
        }
};
}
