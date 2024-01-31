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

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <cstddef>

namespace ns::painter::integrators
{
template <std::size_t N, typename T>
struct Normals final
{
        numerical::Vector<N, T> geometric;
        numerical::Vector<N, T> shading;

        Normals()
        {
        }

        Normals(const numerical::Vector<N, T>& geometric, const numerical::Vector<N, T>& shading)
                : geometric(geometric),
                  shading(shading)
        {
        }
};

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
[[nodiscard]] Normals<N, T> compute_normals(
        const SurfaceIntersection<N, T, Color>& surface,
        const numerical::Vector<N, T>& ray_dir)
{
        const numerical::Vector<N, T> g_normal = surface.geometric_normal();
        ASSERT(g_normal.is_unit());
        const bool flip = dot(ray_dir, g_normal) >= 0;
        const numerical::Vector<N, T> geometric = flip ? -g_normal : g_normal;
        if (!FLAT_SHADING)
        {
                const auto shading = surface.shading_normal();
                if (shading)
                {
                        ASSERT(shading->is_unit());
                        return {geometric, (flip ? -*shading : *shading)};
                }
        }
        return {geometric, geometric};
}
}
