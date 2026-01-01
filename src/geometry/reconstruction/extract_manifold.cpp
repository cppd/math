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

#include "extract_manifold.h"

#include <src/com/error.h>
#include <src/geometry/core/delaunay.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <stack>
#include <vector>

namespace ns::geometry::reconstruction
{
namespace
{
template <std::size_t N>
std::vector<std::vector<int>> find_delaunay_object_facets(
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets)
{
        std::vector<std::vector<int>> res;
        res.resize(delaunay_objects.size());
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                res[delaunay_facets[i].delaunay(0)].push_back(i);
                if (delaunay_facets[i].one_sided())
                {
                        continue;
                }
                res[delaunay_facets[i].delaunay(1)].push_back(i);
        }
        return res;
}

template <std::size_t N>
std::stack<int> find_external_facets(const std::vector<core::DelaunayFacet<N>>& delaunay_facets)
{
        std::stack<int> res;
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!delaunay_facets[i].one_sided())
                {
                        continue;
                }
                res.push(i);
        }
        return res;
}

template <std::size_t N>
std::optional<int> find_index(const core::DelaunayFacet<N>& facet, const std::vector<bool>& visited_delaunay_objects)
{
        if (facet.one_sided())
        {
                if (visited_delaunay_objects[facet.delaunay(0)])
                {
                        return std::nullopt;
                }
                return facet.delaunay(0);
        }

        if (visited_delaunay_objects[facet.delaunay(0)] && visited_delaunay_objects[facet.delaunay(1)])
        {
                return std::nullopt;
        }

        ASSERT(visited_delaunay_objects[facet.delaunay(0)] || visited_delaunay_objects[facet.delaunay(1)]);
        if (visited_delaunay_objects[facet.delaunay(0)])
        {
                return facet.delaunay(1);
        }
        return facet.delaunay(0);
}

template <std::size_t N>
std::optional<int> delaunay_for_facet(
        const core::DelaunayFacet<N>& facet,
        std::vector<bool>* const visited_delaunay_objects)
{
        if (const auto index = find_index(facet, *visited_delaunay_objects))
        {
                (*visited_delaunay_objects)[*index] = true;
                return index;
        }
        return std::nullopt;
}

template <std::size_t N>
std::vector<bool> traverse_delaunay_facets(
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        const std::vector<std::vector<int>>& delaunay_object_facets =
                find_delaunay_object_facets(delaunay_objects, delaunay_facets);

        std::vector<bool> visited_cocone_facets(cocone_facets.size(), false);
        std::vector<bool> visited_delaunay_objects(delaunay_objects.size(), false);

        std::stack<int> next_facets = find_external_facets(delaunay_facets);

        while (!next_facets.empty())
        {
                const int facet = next_facets.top();
                next_facets.pop();

                if (cocone_facets[facet])
                {
                        visited_cocone_facets[facet] = true;
                        continue;
                }

                const std::optional<int> delaunay =
                        delaunay_for_facet(delaunay_facets[facet], &visited_delaunay_objects);
                if (!delaunay)
                {
                        continue;
                }

                for (const int f : delaunay_object_facets[*delaunay])
                {
                        if (f != facet)
                        {
                                next_facets.push(f);
                        }
                }
        }

        return visited_cocone_facets;
}
}

template <std::size_t N>
std::vector<bool> extract_manifold(
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        return traverse_delaunay_facets(delaunay_objects, delaunay_facets, cocone_facets);
}

#define TEMPLATE(N)                                                                                          \
        template std::vector<bool> extract_manifold(                                                         \
                const std::vector<core::DelaunayObject<(N)>>&, const std::vector<core::DelaunayFacet<(N)>>&, \
                const std::vector<bool>&);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
