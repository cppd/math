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

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
std::vector<uint32_t> code_triangles_vert();
std::vector<uint32_t> code_triangles_geom();
std::vector<uint32_t> code_triangles_frag();
std::vector<uint32_t> code_triangles_depth_vert();
std::vector<uint32_t> code_triangle_lines_vert();
std::vector<uint32_t> code_triangle_lines_geom();
std::vector<uint32_t> code_triangle_lines_frag();
std::vector<uint32_t> code_points_0d_vert();
std::vector<uint32_t> code_points_1d_vert();
std::vector<uint32_t> code_points_frag();
std::vector<uint32_t> code_normals_vert();
std::vector<uint32_t> code_normals_geom();
std::vector<uint32_t> code_normals_frag();
std::vector<uint32_t> code_volume_vert();
std::vector<uint32_t> code_volume_image_frag();
std::vector<uint32_t> code_volume_image_fragments_frag();
std::vector<uint32_t> code_volume_fragments_frag();
}
