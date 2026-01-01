/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <cstdint>
#include <span>

namespace ns::gpu::renderer
{
std::span<const std::uint32_t> Code::mesh_normals_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_normals_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.geom.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.geom.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_normals_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_normals.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_normals.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_points_0d_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points_0d.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_0d.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_points_1d_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points_1d.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points_1d.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_points_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_points.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_points.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_shadow_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_shadow.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_shadow.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangle_lines_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangle_lines_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.geom.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.geom.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangle_lines_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangle_lines.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangle_lines.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangles_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangles_geom() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.geom.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.geom.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::mesh_triangles_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_mesh_triangles.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_mesh_triangles.vert.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_image_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_image_opacity_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image_opacity.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image_opacity.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_image_opacity_transparency_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image_opacity_transparency.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image_opacity_transparency.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_image_transparency_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_image_transparency.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_image_transparency.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_opacity_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_opacity.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_opacity.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_opacity_transparency_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_opacity_transparency.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_opacity_transparency.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_transparency_frag() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume_transparency.frag.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume_transparency.frag.spr"
        };
        return CODE;
}

std::span<const std::uint32_t> Code::volume_vert() const
{
        if (ray_tracing_)
        {
                static constexpr std::uint32_t CODE[] = {
#include "ray_tracing/renderer_volume.vert.spr"
                };
                return CODE;
        }

        static constexpr std::uint32_t CODE[] = {
#include "renderer_volume.vert.spr"
        };
        return CODE;
}
}
