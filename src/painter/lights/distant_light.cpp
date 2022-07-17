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

#include "distant_light.h"

#include <src/color/color.h>
#include <src/settings/instantiation.h>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> DistantLight<N, T, Color>::sample(PCG& /*engine*/, const Vector<N, T>& /*point*/) const
{
        return sample_;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> DistantLight<N, T, Color>::info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/)
        const
{
        LightSourceInfo<T, Color> info;
        info.pdf = 0;
        return info;
}

template <std::size_t N, typename T, typename Color>
bool DistantLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
DistantLight<N, T, Color>::DistantLight(const Vector<N, T>& direction, const Color& color)
{
        sample_.l = -direction.normalized();
        sample_.pdf = 1;
        sample_.radiance = color;
}

#define TEMPLATE(N, T, C) template class DistantLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
