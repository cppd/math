/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "shader_source.h"

constexpr uint32_t triangles_vert[]{
#include "renderer_triangles.vert.spr"
};
constexpr uint32_t triangles_geom[]{
#include "renderer_triangles.geom.spr"
};
constexpr uint32_t triangles_frag[]{
#include "renderer_triangles.frag.spr"
};
constexpr uint32_t shadow_vert[]{
#include "renderer_shadow.vert.spr"
};
constexpr uint32_t shadow_frag[]{
#include "renderer_shadow.frag.spr"
};
constexpr uint32_t points_0d_vert[]{
#include "renderer_points_0d.vert.spr"
};
constexpr uint32_t points_1d_vert[]{
#include "renderer_points_1d.vert.spr"
};
constexpr uint32_t points_frag[]{
#include "renderer_points.frag.spr"
};

namespace gpu_vulkan
{
Span<const uint32_t> renderer_triangles_vert()
{
        return Span<const uint32_t>(triangles_vert);
}

Span<const uint32_t> renderer_triangles_geom()
{
        return Span<const uint32_t>(triangles_geom);
}

Span<const uint32_t> renderer_triangles_frag()
{
        return Span<const uint32_t>(triangles_frag);
}

Span<const uint32_t> renderer_shadow_vert()
{
        return Span<const uint32_t>(shadow_vert);
}

Span<const uint32_t> renderer_shadow_frag()
{
        return Span<const uint32_t>(shadow_frag);
}

Span<const uint32_t> renderer_points_0d_vert()
{
        return Span<const uint32_t>(points_0d_vert);
}

Span<const uint32_t> renderer_points_1d_vert()
{
        return Span<const uint32_t>(points_1d_vert);
}

Span<const uint32_t> renderer_points_frag()
{
        return Span<const uint32_t>(points_frag);
}
}
