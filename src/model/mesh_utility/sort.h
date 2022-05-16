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
template <std::size_t N>
void sort_facets_by_material(
        const Mesh<N>& mesh,
        std::vector<int>* const sorted_facet_indices,
        std::vector<int>* const facet_offset,
        std::vector<int>* const facet_count)
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

        *facet_count = std::vector<int>(new_material_size, 0);
        for (const typename Mesh<N>::Facet& facet : mesh.facets)
        {
                int m = material_index(facet.material);
                ++(*facet_count)[m];
        }

        *facet_offset = std::vector<int>(new_material_size, 0);
        for (int i = 0, sum = 0; i < new_material_size; ++i)
        {
                (*facet_offset)[i] = sum;
                sum += (*facet_count)[i];
        }

        sorted_facet_indices->resize(mesh.facets.size());
        std::vector<int> starting_indices = *facet_offset;
        for (std::size_t i = 0; i < mesh.facets.size(); ++i)
        {
                const int m = material_index(mesh.facets[i].material);
                (*sorted_facet_indices)[starting_indices[m]++] = i;
        }

        ASSERT(facet_offset->size() == facet_count->size());
        ASSERT(facet_count->size() == mesh.materials.size() + 1);
        ASSERT(sorted_facet_indices->size() == mesh.facets.size());
        ASSERT(sorted_facet_indices->size() == sort_and_unique(*sorted_facet_indices).size());
        ASSERT(std::is_sorted(
                std::cbegin(*sorted_facet_indices), std::cend(*sorted_facet_indices),
                [&](int a, int b)
                {
                        int a_ = material_index(mesh.facets[a].material);
                        int b_ = material_index(mesh.facets[b].material);
                        return a_ < b_;
                }));
}
}
