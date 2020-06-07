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
std::vector<uint32_t> code_triangles_vert()
{
        return {
#include "renderer_triangles.vert.spr"
        };
}

std::vector<uint32_t> code_triangles_geom()
{
        return {
#include "renderer_triangles.geom.spr"
        };
}

std::vector<uint32_t> code_triangles_frag()
{
        return {
#include "renderer_triangles.frag.spr"
        };
}

std::vector<uint32_t> code_triangles_depth_vert()
{
        return {
#include "renderer_triangles_depth.vert.spr"
        };
}

std::vector<uint32_t> code_triangle_lines_vert()
{
        return {
#include "renderer_triangle_lines.vert.spr"
        };
}

std::vector<uint32_t> code_triangle_lines_geom()
{
        return {
#include "renderer_triangle_lines.geom.spr"
        };
}

std::vector<uint32_t> code_triangle_lines_frag()
{
        return {
#include "renderer_triangle_lines.frag.spr"
        };
}

std::vector<uint32_t> code_points_0d_vert()
{
        return {
#include "renderer_points_0d.vert.spr"
        };
}

std::vector<uint32_t> code_points_1d_vert()
{
        return {
#include "renderer_points_1d.vert.spr"
        };
}

std::vector<uint32_t> code_points_frag()
{
        return {
#include "renderer_points.frag.spr"
        };
}

std::vector<uint32_t> code_normals_vert()
{
        return {
#include "renderer_normals.vert.spr"
        };
}

std::vector<uint32_t> code_normals_geom()
{
        return {
#include "renderer_normals.geom.spr"
        };
}

std::vector<uint32_t> code_normals_frag()
{
        return {
#include "renderer_normals.frag.spr"
        };
}

std::vector<uint32_t> code_volume_vert()
{
        return {
#include "renderer_volume.vert.spr"
        };
}

std::vector<uint32_t> code_volume_frag()
{
        return {
#include "renderer_volume.frag.spr"
        };
}
}
