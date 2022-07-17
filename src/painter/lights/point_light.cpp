/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "point_light.h"

#include "com/functions.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/settings/instantiation.h>

#include <cmath>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> PointLight<N, T, Color>::sample(PCG& /*engine*/, const Vector<N, T>& point) const
{
        const Vector<N, T> direction = location_ - point;
        const T squared_distance = direction.norm_squared();
        const T distance = std::sqrt(squared_distance);
        const T coef = coef_ / com::power_n1<N>(squared_distance, distance);

        LightSourceSample<N, T, Color> s;
        s.distance = distance;
        s.l = direction / distance;
        s.pdf = 1;
        s.radiance = color_ * coef;
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> PointLight<N, T, Color>::info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const
{
        LightSourceInfo<T, Color> info;
        info.pdf = 0;
        return info;
}

template <std::size_t N, typename T, typename Color>
bool PointLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
PointLight<N, T, Color>::PointLight(
        const Vector<N, T>& location,
        const Color& color,
        const std::type_identity_t<T> unit_intensity_distance)
        : location_(location),
          color_(color),
          coef_(power<N - 1>(unit_intensity_distance))
{
        if (!(unit_intensity_distance > 0))
        {
                error("Error unit intensity distance " + to_string(unit_intensity_distance));
        }
}

#define TEMPLATE(N, T, C) template class PointLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
