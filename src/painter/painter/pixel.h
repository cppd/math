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

namespace ns::painter
{
template <typename Color>
class Pixel final
{
        using DataType = typename Color::DataType;

        Color m_color_sum{0};
        DataType m_hit_weight_sum{0};
        DataType m_background_weight_sum{0};

public:
        void merge(const Color& color_sum, DataType hit_weight_sum, DataType background_weight_sum)
        {
                m_color_sum += color_sum;
                m_hit_weight_sum += hit_weight_sum;
                m_background_weight_sum += background_weight_sum;
        }

        struct Info final
        {
                Color color;
                DataType alpha;
        };

        Info info() const
        {
                Info info;
                const DataType sum = m_hit_weight_sum + m_background_weight_sum;
                if (sum > 0)
                {
                        info.color = m_color_sum / sum;
                        info.alpha = 1 - m_background_weight_sum / sum;
                }
                else
                {
                        info.color = Color(0);
                        info.alpha = 0;
                }
                return info;
        }
};
}
