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

#include "../objects.h"

#include <src/com/math.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <type_traits>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class PointLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> m_location;
        Color m_color;
        T m_coef;

public:
        PointLight(const Vector<N, T>& location, const Color& color, T unit_intensity_distance)
                : m_location(location), m_color(color), m_coef(std::pow(unit_intensity_distance, T(N - 1)))
        {
        }

        LightSourceSample<N, T, Color> sample(const Vector<N, T>& point) const override
        {
                const Vector<N, T> direction = m_location - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);

                T coef = m_coef;

                if constexpr ((N & 1) == 1)
                {
                        coef /= power<((N - 1) / 2)>(squared_distance);
                }
                else
                {
                        coef /= power<((N - 2) / 2)>(squared_distance) * distance;
                }

                LightSourceSample<N, T, Color> s;
                s.distance = distance;
                s.l = direction / distance;
                s.pdf = 1;
                s.L = m_color * coef;
                return s;
        }
};
}
