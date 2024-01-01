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

#include <cstdint>
#include <span>

namespace ns::gpu::renderer
{
class Code final
{
        bool ray_tracing_;

public:
        explicit Code(const bool ray_tracing)
                : ray_tracing_(ray_tracing)
        {
        }

        [[nodiscard]] bool ray_tracing() const
        {
                return ray_tracing_;
        }

        [[nodiscard]] std::span<const std::uint32_t> mesh_normals_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_normals_geom() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_normals_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_points_0d_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_points_1d_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_points_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_shadow_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangle_lines_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangle_lines_geom() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangle_lines_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangles_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangles_geom() const;
        [[nodiscard]] std::span<const std::uint32_t> mesh_triangles_vert() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_image_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_image_opacity_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_image_opacity_transparency_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_image_transparency_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_opacity_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_opacity_transparency_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_transparency_frag() const;
        [[nodiscard]] std::span<const std::uint32_t> volume_vert() const;
};
}
