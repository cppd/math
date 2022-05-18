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

#include "code.h"

namespace ns::gpu::renderer
{
class ShaderCode final : public Code
{
        bool ray_tracing_;

        [[nodiscard]] bool ray_tracing() const override
        {
                return ray_tracing_;
        }

        [[nodiscard]] std::vector<std::uint32_t> mesh_triangles_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_triangles_geom() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_triangles_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_shadow_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_triangle_lines_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_triangle_lines_geom() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_triangle_lines_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_points_0d_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_points_1d_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_points_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_normals_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_normals_geom() const override;
        [[nodiscard]] std::vector<std::uint32_t> mesh_normals_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_vert() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_image_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_image_fragments_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_fragments_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_image_opacity_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_image_fragments_opacity_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_fragments_opacity_frag() const override;
        [[nodiscard]] std::vector<std::uint32_t> volume_opacity_frag() const override;

public:
        explicit ShaderCode(bool ray_tracing);
};
}
