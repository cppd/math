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

#include "shader_code.h"

namespace ns::gpu::renderer
{
std::vector<std::uint32_t> ShaderCode::mesh_triangles_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangles_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.geom.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangles_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangles_image_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles_image.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles_image.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_shadow_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_shadow.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_shadow.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangle_lines_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangle_lines_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.geom.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_triangle_lines_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_points_0d_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points_0d.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_0d.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_points_1d_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points_1d.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_1d.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_points_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_normals_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_normals_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.geom.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.geom.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::mesh_normals_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::volume_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume.vert.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::volume_image_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::volume_image_fragments_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image_fragments.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image_fragments.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> ShaderCode::volume_fragments_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_fragments.frag.spr"
                };
                return {std::cbegin(CODE), std::cend(CODE)};
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_fragments.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

ShaderCode::ShaderCode(const bool ray_tracing) : ray_tracing_(ray_tracing)
{
}
}
