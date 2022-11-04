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

/*
Robert Sedgewick, Kevin Wayne.
Algorithms. Fourth edition.
Pearson Education, 2011.

5.1 String Sorts
Key-indexed counting
*/

#pragma once

#include "../mesh.h"

#include <src/com/alg.h>

#include <algorithm>
#include <vector>

namespace ns::model::mesh
{
struct SortedFacets final
{
        std::vector<int> indices;
        std::vector<int> offset;
        std::vector<int> count;
};

template <std::size_t N>
SortedFacets sort_facets_by_material(const Mesh<N>& mesh)
{
        ASSERT(std::all_of(
                std::cbegin(mesh.facets), std::cend(mesh.facets),
                [&](const typename Mesh<N>::Facet& facet)
                {
                        return facet.material < static_cast<int>(mesh.materials.size());
                }));

        // for facets without material there is an additional material at the end
        const int max_material_index = mesh.materials.size();
        const int new_material_size = mesh.materials.size() + 1;

        const auto material_index = [&](int i)
        {
                return i >= 0 ? i : max_material_index;
        };

        SortedFacets facets;

        facets.count = std::vector<int>(new_material_size, 0);
        for (const typename Mesh<N>::Facet& facet : mesh.facets)
        {
                int m = material_index(facet.material);
                ++facets.count[m];
        }

        facets.offset = std::vector<int>(new_material_size, 0);
        for (int i = 0, sum = 0; i < new_material_size; ++i)
        {
                facets.offset[i] = sum;
                sum += facets.count[i];
        }

        facets.indices.resize(mesh.facets.size());
        std::vector<int> starting_indices = facets.offset;
        for (std::size_t i = 0; i < mesh.facets.size(); ++i)
        {
                const int m = material_index(mesh.facets[i].material);
                facets.indices[starting_indices[m]++] = i;
        }

        ASSERT(facets.offset.size() == facets.count.size());
        ASSERT(facets.count.size() == mesh.materials.size() + 1);
        ASSERT(facets.indices.size() == mesh.facets.size());
        ASSERT(facets.indices.size() == sort_and_unique(facets.indices).size());
        ASSERT(std::is_sorted(
                std::cbegin(facets.indices), std::cend(facets.indices),
                [&](int a, int b)
                {
                        int a_ = material_index(mesh.facets[a].material);
                        int b_ = material_index(mesh.facets[b].material);
                        return a_ < b_;
                }));

        return facets;
}
}
