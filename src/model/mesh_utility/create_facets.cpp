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

#include "create_facets.h"

#include "normals.h"
#include "position.h"

#include <src/com/error.h>

#include <unordered_map>

namespace mesh
{
namespace
{
template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        std::unordered_map<int, int> vertices;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                for (int vertex_index : facet)
                {
                        auto [iter, inserted] = vertices.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());
        for (const auto& [old_index, new_index] : vertices)
        {
                mesh->vertices[new_index] = points[old_index];
        }

        mesh->facets.reserve(facets.size());
        for (const std::array<int, N>& facet : facets)
        {
                typename Mesh<N>::Facet mesh_facet;

                mesh_facet.material = -1;
                mesh_facet.has_texcoord = false;
                mesh_facet.has_normal = false;

                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = vertices.find(facet[i]);
                        ASSERT(iter != vertices.cend());
                        mesh_facet.vertices[i] = iter->second;
                        mesh_facet.normals[i] = -1;
                        mesh_facet.texcoords[i] = -1;
                }

                mesh->facets.push_back(std::move(mesh_facet));
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        std::unique_ptr<Mesh<N>> mesh = create_mesh(points, facets);
        compute_normals(mesh.get());
        return mesh;
}

template std::unique_ptr<Mesh<3>> create_mesh_for_facets(
        const std::vector<Vector<3, float>>& points,
        const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<Mesh<4>> create_mesh_for_facets(
        const std::vector<Vector<4, float>>& points,
        const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<Mesh<5>> create_mesh_for_facets(
        const std::vector<Vector<5, float>>& points,
        const std::vector<std::array<int, 5>>& facets);
template std::unique_ptr<Mesh<6>> create_mesh_for_facets(
        const std::vector<Vector<6, float>>& points,
        const std::vector<std::array<int, 6>>& facets);
}
