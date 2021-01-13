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

#include <algorithm>
#include <tuple>

namespace ns::gpu::renderer
{
inline std::tuple<float, float, float> clean_shading_parameters(float ambient, float metalness, float roughness)
{
        ambient = std::clamp(ambient, float(0), float(1));
        metalness = std::clamp(metalness, float(0), float(1));
        roughness = std::clamp(roughness, float(0), float(1));
        return {ambient, metalness, roughness};
}
}
