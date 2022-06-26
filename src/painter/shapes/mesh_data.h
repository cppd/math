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

#include "mesh_facet.h"
#include "mesh_material.h"
#include "mesh_texture.h"

#include <src/com/error.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/model/mesh_object.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class MeshData final
{
        std::vector<Vector<N, T>> vertices_;
        std::vector<Vector<N, T>> normals_;
        std::vector<Vector<N - 1, T>> texcoords_;
        std::vector<MeshMaterial<T, Color>> materials_;
        std::vector<MeshTexture<N - 1>> images_;
        std::vector<MeshFacet<N, T>> facets_;
        std::vector<std::array<int, N>> facet_vertex_indices_;

        void create(const model::mesh::Reading<N>& mesh_object);

        void create(
                const std::vector<model::mesh::Reading<N>>& mesh_objects,
                const std::optional<Vector<N + 1, T>>& clip_plane_equation);

public:
        MeshData(
                const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
                const std::optional<Vector<N + 1, T>>& clip_plane_equation,
                bool write_log);

        [[nodiscard]] const std::vector<Vector<N, T>>& normals() const
        {
                return normals_;
        }

        [[nodiscard]] const std::vector<Vector<N - 1, T>>& texcoords() const
        {
                return texcoords_;
        }

        [[nodiscard]] const std::vector<MeshMaterial<T, Color>>& materials() const
        {
                return materials_;
        }

        [[nodiscard]] const std::vector<MeshTexture<N - 1>>& images() const
        {
                return images_;
        }

        [[nodiscard]] const std::vector<MeshFacet<N, T>>& facets() const
        {
                return facets_;
        }

        [[nodiscard]] geometry::BoundingBox<N, T> facet_bounding_box(const std::size_t facet_index) const
        {
                ASSERT(facet_index < facet_vertex_indices_.size());
                return geometry::BoundingBox(vertices_, facet_vertex_indices_[facet_index]);
        }
};
}
