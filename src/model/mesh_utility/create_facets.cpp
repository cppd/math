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

#include "create_facets.h"

#include "normals.h"
#include "position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

#include <unordered_map>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N>
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
                for (const int vertex_index : facet)
                {
                        const auto [iter, inserted] = vertices.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        auto mesh = std::make_unique<Mesh<N>>();

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

                for (std::size_t i = 0; i < N; ++i)
                {
                        const auto iter = vertices.find(facet[i]);
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

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets,
        const bool write_log)
{
        std::unique_ptr<Mesh<N>> mesh;
        {
                const Clock::time_point start_time = Clock::now();

                mesh = create_mesh(points, facets);

                if (write_log)
                {
                        LOG("Mesh created, " + to_string_fixed(duration_from(start_time), 5) + " s");
                }
        }
        {
                const Clock::time_point start_time = Clock::now();

                compute_normals(mesh.get());

                if (write_log)
                {
                        LOG("Mesh normals computed, " + to_string_fixed(duration_from(start_time), 5) + " s");
                }
        }
        return mesh;
}

#define CREATE_MESH_FOR_FACETS_INSTANTIATION(N)                     \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_facets( \
                const std::vector<Vector<(N), float>>&, const std::vector<std::array<int, (N)>>&, bool);

CREATE_MESH_FOR_FACETS_INSTANTIATION(3)
CREATE_MESH_FOR_FACETS_INSTANTIATION(4)
CREATE_MESH_FOR_FACETS_INSTANTIATION(5)
CREATE_MESH_FOR_FACETS_INSTANTIATION(6)
}
