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

#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace ns::geometry::shapes
{
template <std::size_t N, typename T>
void create_mesh(
        const std::vector<std::array<numerical::Vector<N, T>, N>>& facets,
        std::vector<numerical::Vector<N, T>>* const mesh_vertices,
        std::vector<std::array<int, N>>* const mesh_facets)
{
        mesh_vertices->clear();
        mesh_facets->clear();
        mesh_facets->reserve(facets.size());

        std::unordered_map<numerical::Vector<N, T>, int> map;
        map.reserve(N * facets.size());

        for (const std::array<numerical::Vector<N, T>, N>& vertices : facets)
        {
                std::array<int, N>& mesh_facet = mesh_facets->emplace_back();
                for (std::size_t i = 0; i < N; ++i)
                {
                        auto iter = map.find(vertices[i]);
                        if (iter == map.cend())
                        {
                                iter = map.emplace(vertices[i], mesh_vertices->size()).first;
                                mesh_vertices->push_back(vertices[i]);
                        }
                        mesh_facet[i] = iter->second;
                }
        }
}
}
