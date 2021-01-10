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

namespace ns::gpu::renderer
{
struct ShadingParameters
{
        float ambient;
        float metalness;
        float specular_power;
};

inline ShadingParameters shading_parameters(float ambient, float diffuse, float specular, float specular_power)
{
        ShadingParameters p;

        p.ambient = std::clamp(ambient, float(0), float(1));

        diffuse = std::max(float(0), diffuse);
        specular = std::max(float(0), specular);
        const float sum = diffuse + specular;
        p.metalness = (sum != 0) ? specular / sum : float(0.5);

        p.specular_power = std::max(float(1), specular_power);

        return p;
}
}
