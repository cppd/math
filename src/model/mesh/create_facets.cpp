/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/model/mesh.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N>
std::unordered_map<int, int> vertex_index_map(const std::vector<std::array<int, N>>& facets)
{
        std::unordered_map<int, int> res;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                for (const int vertex_index : facet)
                {
                        const auto [iter, inserted] = res.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(res.size()));

        return res;
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<numerical::Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        const std::unordered_map<int, int> vertex_map = vertex_index_map(facets);

        auto mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertex_map.size());
        for (const auto& [old_index, new_index] : vertex_map)
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
                        const auto iter = vertex_map.find(facet[i]);
                        ASSERT(iter != vertex_map.cend());
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
        const std::vector<numerical::Vector<N, float>>& points,
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

#define TEMPLATE(N)                                                 \
        template std::unique_ptr<Mesh<(N)>> create_mesh_for_facets( \
                const std::vector<numerical::Vector<(N), float>>&, const std::vector<std::array<int, (N)>>&, bool);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
