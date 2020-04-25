/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "code.h"

namespace gpu::renderer
{
namespace
{
constexpr uint32_t triangles_vert[]{
#include "renderer_triangles.vert.spr"
};
constexpr uint32_t triangles_geom[]{
#include "renderer_triangles.geom.spr"
};
constexpr uint32_t triangles_frag[]{
#include "renderer_triangles.frag.spr"
};
constexpr uint32_t triangles_depth_vert[]{
#include "renderer_triangles_depth.vert.spr"
};
constexpr uint32_t triangle_lines_vert[]{
#include "renderer_triangle_lines.vert.spr"
};
constexpr uint32_t triangle_lines_geom[]{
#include "renderer_triangle_lines.geom.spr"
};
constexpr uint32_t triangle_lines_frag[]{
#include "renderer_triangle_lines.frag.spr"
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
constexpr uint32_t normals_vert[]{
#include "renderer_normals.vert.spr"
};
constexpr uint32_t normals_geom[]{
#include "renderer_normals.geom.spr"
};
constexpr uint32_t normals_frag[]{
#include "renderer_normals.frag.spr"
};
constexpr uint32_t volume_vert[]{
#include "renderer_volume.vert.spr"
};
constexpr uint32_t volume_frag[]{
#include "renderer_volume.frag.spr"
};
}

Span<const uint32_t> code_triangles_vert()
{
        return Span<const uint32_t>(triangles_vert);
}

Span<const uint32_t> code_triangles_geom()
{
        return Span<const uint32_t>(triangles_geom);
}

Span<const uint32_t> code_triangles_frag()
{
        return Span<const uint32_t>(triangles_frag);
}

Span<const uint32_t> code_triangles_depth_vert()
{
        return Span<const uint32_t>(triangles_depth_vert);
}

Span<const uint32_t> code_triangle_lines_vert()
{
        return Span<const uint32_t>(triangle_lines_vert);
}

Span<const uint32_t> code_triangle_lines_geom()
{
        return Span<const uint32_t>(triangle_lines_geom);
}

Span<const uint32_t> code_triangle_lines_frag()
{
        return Span<const uint32_t>(triangle_lines_frag);
}

Span<const uint32_t> code_points_0d_vert()
{
        return Span<const uint32_t>(points_0d_vert);
}

Span<const uint32_t> code_points_1d_vert()
{
        return Span<const uint32_t>(points_1d_vert);
}

Span<const uint32_t> code_points_frag()
{
        return Span<const uint32_t>(points_frag);
}

Span<const uint32_t> code_normals_vert()
{
        return Span<const uint32_t>(normals_vert);
}

Span<const uint32_t> code_normals_geom()
{
        return Span<const uint32_t>(normals_geom);
}

Span<const uint32_t> code_normals_frag()
{
        return Span<const uint32_t>(normals_frag);
}

Span<const uint32_t> code_volume_vert()
{
        return Span<const uint32_t>(volume_vert);
}

Span<const uint32_t> code_volume_frag()
{
        return Span<const uint32_t>(volume_frag);
}
}
