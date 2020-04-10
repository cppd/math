/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/color/color.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <type_traits>

namespace painter
{
template <size_t N, typename T>
class PointLight
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> m_location;
        Color m_color;
        T m_coef;

public:
        PointLight(const Vector<N, T>& location, const Color& color, T unit_intensity_distance)
                : m_location(location), m_color(color), m_coef(std::pow(unit_intensity_distance, N - 1))
        {
        }

        void properties(const Vector<N, T>& point, Color* color, Vector<N, T>* vector_from_point_to_light) const
        {
                *vector_from_point_to_light = m_location - point;

                T square_distance = dot(*vector_from_point_to_light, *vector_from_point_to_light);

                if constexpr (N == 3)
                {
                        *color = m_color * (m_coef / square_distance);
                }
                else
                {
                        *color = m_color * (m_coef / std::pow(square_distance, T(N - 1) / 2));
                }
        }
};

template <size_t N, typename T>
class ConstantLight
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> m_location;
        Color m_color;

public:
        ConstantLight(const Vector<N, T>& location, const Color& color) : m_location(location), m_color(color)
        {
        }

        void properties(const Vector<N, T>& point, Color* color, Vector<N, T>* vector_from_point_to_light) const
        {
                *vector_from_point_to_light = m_location - point;
                *color = m_color;
        }
};
}
