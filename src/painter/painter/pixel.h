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

#include <tuple>

namespace ns::painter
{
template <typename Color>
class Pixel final
{
        using DataType = typename Color::DataType;

        Color m_color_sum{0};
        DataType m_color_weight_sum{0};
        DataType m_background_weight_sum{0};

public:
        void merge(const Color& color_sum, DataType color_weight_sum, DataType background_weight_sum)
        {
                m_color_sum += color_sum;
                m_color_weight_sum += color_weight_sum;
                m_background_weight_sum += background_weight_sum;
        }

        bool has_color() const
        {
                return m_color_weight_sum > 0;
        }

        Color color(const Color& background_color) const
        {
                ASSERT(has_color());
                if (m_background_weight_sum == 0)
                {
                        return m_color_sum / m_color_weight_sum;
                }
                const DataType sum = m_color_weight_sum + m_background_weight_sum;
                return (m_color_sum + m_background_weight_sum * background_color) / sum;
        }

        bool has_color_alpha() const
        {
                return m_color_weight_sum > 0;
        }

        std::tuple<Color, DataType> color_alpha() const
        {
                ASSERT(has_color_alpha());
                if (m_background_weight_sum == 0)
                {
                        return {m_color_sum / m_color_weight_sum, 1};
                }
                const DataType sum = m_color_weight_sum + m_background_weight_sum;
                return {m_color_sum / sum, m_color_weight_sum / sum};
        }
};
}
