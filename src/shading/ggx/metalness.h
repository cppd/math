/*
Copyright (C) 2017-2025 Topological Manifold

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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5.2 Typical Fresnel Reflectance Values
Parameterizing Fresnel Values
*/

#pragma once

#include <src/shading/objects.h>

namespace ns::shading::ggx
{
template <typename Color, typename T>
Colors<Color> compute_metalness(const Color& surface_color, const T metalness)
{
        static constexpr Color F0(0.05);
        static constexpr Color BLACK(0);

        Colors<Color> res;
        res.f0 = interpolation(F0, surface_color, metalness);
        res.rho_ss = interpolation(surface_color, BLACK, metalness);
        return res;
}
}
