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

#include <src/color/color.h>
#include <src/numerical/vec.h>

#include <type_traits>

namespace ns::painter
{
template <std::size_t N, typename T>
class DistantLight final : public LightSource<N, T>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        LightSourceSample<N, T> m_sample;

public:
        DistantLight(const Vector<N, T>& direction, const Color& color)
        {
                m_sample.l = direction.normalized();
                m_sample.pdf = 1;
                m_sample.color = color;
        }

        LightSourceSample<N, T> sample(const Vector<N, T>& /*point*/) const override
        {
                return m_sample;
        }
};
}
