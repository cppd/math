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

#pragma once

#include <src/numerical/region.h>
#include <src/numerical/vector.h>

namespace ns::gpu::renderer
{
struct ViewportTransform final
{
        // device_coordinates = (framebuffer_coordinates - center) * factor
        numerical::Vector2d center;
        numerical::Vector2d factor;
};

inline ViewportTransform viewport_transform(const numerical::Region<2, int>& viewport)
{
        const numerical::Vector2d offset = to_vector<double>(viewport.from());
        const numerical::Vector2d extent = to_vector<double>(viewport.extent());

        ViewportTransform res;
        res.center = offset + 0.5 * extent;
        res.factor = numerical::Vector2d(2.0 / extent[0], 2.0 / extent[1]);
        return res;
}
}
