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

#pragma once

#include <src/com/span.h>

#include <cstdint>

namespace gpu::renderer
{
Span<const uint32_t> code_triangles_vert();
Span<const uint32_t> code_triangles_geom();
Span<const uint32_t> code_triangles_frag();
Span<const uint32_t> code_triangles_depth_vert();
Span<const uint32_t> code_triangle_lines_vert();
Span<const uint32_t> code_triangle_lines_geom();
Span<const uint32_t> code_triangle_lines_frag();
Span<const uint32_t> code_points_0d_vert();
Span<const uint32_t> code_points_1d_vert();
Span<const uint32_t> code_points_frag();
Span<const uint32_t> code_normals_vert();
Span<const uint32_t> code_normals_geom();
Span<const uint32_t> code_normals_frag();
Span<const uint32_t> code_volume_vert();
Span<const uint32_t> code_volume_frag();
}
