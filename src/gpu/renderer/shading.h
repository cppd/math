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
inline std::tuple<float, float, float, float> prepare_shading_parameters(
        float ambient,
        float diffuse,
        float specular,
        float specular_power)
{
        ambient = std::max(float(0), ambient);
        diffuse = std::max(float(0), diffuse);
        specular = std::max(float(0), specular);
        const float sum = ambient + diffuse + specular;
        if (sum != 0)
        {
                ambient /= sum;
                diffuse /= sum;
                specular /= sum;
        }
        else
        {
                ambient = 1.0 / 3;
                diffuse = 1.0 / 3;
                specular = 1.0 / 3;
        }

        specular_power = std::max(float(1), specular_power);

        return {ambient, diffuse, specular, specular_power};
}
}
