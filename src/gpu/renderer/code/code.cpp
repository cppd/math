/*
Copyright (C) 2017-2022 Topological Manifold

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

namespace ns::gpu::renderer
{
std::vector<std::uint32_t> code_mesh_triangles_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangles_geom()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangles_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangles_depth_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles_depth.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangle_lines_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangle_lines_geom()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_triangle_lines_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_points_0d_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_0d.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_points_1d_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_1d.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_points_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_normals_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_normals_geom()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_mesh_normals_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_volume_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_volume_image_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_volume_image_fragments_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image_fragments.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_volume_fragments_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_fragments.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}
}
