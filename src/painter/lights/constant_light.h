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

#include "../objects.h"

#include <src/color/color.h>
#include <src/numerical/vec.h>

#include <type_traits>

namespace ns::painter
{
template <size_t N, typename T>
class ConstantLight final : public LightSource<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> m_location;
        Color m_color;

public:
        ConstantLight(const Vector<N, T>& location, const Color& color) : m_location(location), m_color(color)
        {
        }

        LightProperties<N, T> properties(const Vector<N, T>& point) const override
        {
                LightProperties<N, T> p;
                p.direction_to_light = m_location - point;
                p.color = m_color;
                return p;
        }
};
}
