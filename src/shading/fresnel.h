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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5 Fresnel Reflectance
*/

#pragma once

#include <src/com/error.h>
#include <src/com/exponent.h>

namespace ns::shading
{
// (9.16)
// Schlick approximation of Fresnel reflectance
template <typename T, typename Color>
Color fresnel(const Color& f0, const T h_l)
{
        ASSERT(h_l >= 0);
        static constexpr Color WHITE(1);
        return interpolation(f0, WHITE, power<5>(1 - h_l));
}
}
