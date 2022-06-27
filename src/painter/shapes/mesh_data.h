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

#include <src/model/mesh_object.h>

#include <array>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
struct MeshData final
{
        std::vector<Vector<N, T>> vertices;
        std::vector<Vector<N, T>> normals;
        std::vector<Vector<N - 1, T>> texcoords;
        std::vector<MeshMaterial<T, Color>> materials;
        std::vector<MeshTexture<N - 1>> images;
        std::vector<MeshFacet<N, T>> facets;
};

template <std::size_t N, typename T, typename Color>
struct MeshInfo final
{
        MeshData<N, T, Color> mesh_data;
        std::vector<std::array<int, N>> facet_vertex_indices;
};

template <std::size_t N, typename T, typename Color>
MeshInfo<N, T, Color> create_mesh_info(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        bool write_log);
}
