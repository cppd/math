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

#pragma once

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
class Code
{
protected:
        ~Code() = default;

public:
        virtual bool ray_tracing() const = 0;

        virtual std::vector<std::uint32_t> mesh_triangles_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangles_geom() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangles_frag() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangles_image_frag() const = 0;
        virtual std::vector<std::uint32_t> mesh_shadow_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangle_lines_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangle_lines_geom() const = 0;
        virtual std::vector<std::uint32_t> mesh_triangle_lines_frag() const = 0;
        virtual std::vector<std::uint32_t> mesh_points_0d_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_points_1d_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_points_frag() const = 0;
        virtual std::vector<std::uint32_t> mesh_normals_vert() const = 0;
        virtual std::vector<std::uint32_t> mesh_normals_geom() const = 0;
        virtual std::vector<std::uint32_t> mesh_normals_frag() const = 0;
        virtual std::vector<std::uint32_t> volume_vert() const = 0;
        virtual std::vector<std::uint32_t> volume_image_frag() const = 0;
        virtual std::vector<std::uint32_t> volume_image_fragments_frag() const = 0;
        virtual std::vector<std::uint32_t> volume_fragments_frag() const = 0;
};
}
