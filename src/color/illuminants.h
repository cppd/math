/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "color.h"

namespace ns::color
{
[[nodiscard]] const Spectrum& daylight_d65();

[[nodiscard]] double daylight_min_cct();
[[nodiscard]] double daylight_max_cct();
[[nodiscard]] Spectrum daylight(double cct);

[[nodiscard]] const Spectrum& blackbody_a();
[[nodiscard]] Spectrum blackbody(double t);
}
