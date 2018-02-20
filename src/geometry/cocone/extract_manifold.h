/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/error.h"
#include "geometry/core/delaunay.h"

#include <stack>
#include <vector>

namespace ExtractManifoldImplementation
{
template <size_t N>
void find_delaunay_object_facets(const std::vector<DelaunayObject<N>>& delaunay_objects,
                                 const std::vector<DelaunayFacet<N>>& delaunay_facets,
                                 std::vector<std::vector<int>>* delaunay_object_facets)
{
        delaunay_object_facets->clear();
        delaunay_object_facets->resize(delaunay_objects.size());

        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                (*delaunay_object_facets)[delaunay_facets[i].delaunay(0)].push_back(i);
                if (delaunay_facets[i].one_sided())
                {
                        continue;
                }
                (*delaunay_object_facets)[delaunay_facets[i].delaunay(1)].push_back(i);
        }
}

// Выборка только внешних граней COCONE.
// Проход по граням Делоне через объекты Делоне, начиная от самых внешних граней.
// При встречи грани COCONE она помечается как нужная, и за неё идти не надо.
template <size_t N>
void traverse_delaunay(const std::vector<DelaunayFacet<N>>& delaunay_facets,
                       const std::vector<std::vector<int>>& delaunay_object_facets, const std::vector<bool>& cocone_facets,
                       std::vector<bool>* visited_delaunay, std::vector<bool>* visited_cocone_facets)
{
        std::stack<int> next;

        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                // Надо начинать обход с внешних граней
                if (!delaunay_facets[i].one_sided())
                {
                        continue;
                }
                next.push(i);
        }

        while (!next.empty())
        {
                int facet_index = next.top();
                next.pop();

                if (cocone_facets[facet_index])
                {
                        (*visited_cocone_facets)[facet_index] = true;
                        continue;
                }

                const DelaunayFacet<N>& facet = delaunay_facets[facet_index];

                int delaunay_index;
                if (facet.one_sided())
                {
                        if ((*visited_delaunay)[facet.delaunay(0)])
                        {
                                continue;
                        }

                        delaunay_index = facet.delaunay(0);
                }
                else
                {
                        if ((*visited_delaunay)[facet.delaunay(0)] && (*visited_delaunay)[facet.delaunay(1)])
                        {
                                continue;
                        }

                        ASSERT((*visited_delaunay)[facet.delaunay(0)] || (*visited_delaunay)[facet.delaunay(1)]);

                        delaunay_index = (*visited_delaunay)[facet.delaunay(0)] ? facet.delaunay(1) : facet.delaunay(0);
                }

                (*visited_delaunay)[delaunay_index] = true;

                for (int f : delaunay_object_facets[delaunay_index])
                {
                        if (f != facet_index)
                        {
                                next.push(f);
                        }
                }
        }
}
}

template <size_t N>
void extract_manifold(const std::vector<DelaunayObject<N>>& delaunay_objects,
                      const std::vector<DelaunayFacet<N>>& delaunay_facets, std::vector<bool>* cocone_facets)
{
        using namespace ExtractManifoldImplementation;

        std::vector<std::vector<int>> delaunay_object_facets;
        std::vector<bool> visited_delaunay(delaunay_objects.size(), false);
        std::vector<bool> visited_cocone_facets(cocone_facets->size(), false);

        find_delaunay_object_facets(delaunay_objects, delaunay_facets, &delaunay_object_facets);

        traverse_delaunay(delaunay_facets, delaunay_object_facets, *cocone_facets, &visited_delaunay, &visited_cocone_facets);

        *cocone_facets = std::move(visited_cocone_facets);
}
