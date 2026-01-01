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

#pragma once

#include "facet.h"
#include "material.h"
#include "texture.h"

#include <src/model/mesh_object.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::shapes::mesh
{
template <std::size_t N, typename T, typename Color>
struct Mesh final
{
        std::vector<numerical::Vector<N, T>> vertices;
        std::vector<numerical::Vector<N, T>> normals;
        std::vector<numerical::Vector<N - 1, T>> texcoords;
        std::vector<Material<T, Color>> materials;
        std::vector<Texture<N - 1>> images;
        std::vector<Facet<N, T>> facets;
};

template <std::size_t N, typename T, typename Color>
struct MeshData final
{
        Mesh<N, T, Color> mesh;
        std::vector<std::array<int, N>> facet_vertex_indices;
};

template <std::size_t N, typename T, typename Color>
MeshData<N, T, Color> create_mesh_data(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        bool write_log);
}
