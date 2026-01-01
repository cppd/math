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

#include "print.h"

#include "structure.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/geometry/core/delaunay.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <string>
#include <vector>

namespace ns::geometry::reconstruction
{
template <std::size_t N>
void print_delaunay_facets(const std::vector<core::DelaunayFacet<N>>& delaunay_facets)
{
        std::string s;
        s += "-- delaunay facets --\n";
        for (const core::DelaunayFacet<N>& facet : delaunay_facets)
        {
                s += to_string(facet.vertices());
                s += '\n';
        }
        s += "--";
        LOG(s);
}

template <std::size_t N>
void print_cocone_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        std::string s;
        s += "-- cocone facets --\n";
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if (cocone_facets[i])
                {
                        s += to_string(delaunay_facets[i].vertices());
                        s += '\n';
                }
        }
        s += "--";
        LOG(s);
}

template <std::size_t N>
void print_non_cocone_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        std::string s;
        s += "-- non-cocone facets --\n";
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        s += to_string(delaunay_facets[i].vertices());
                        s += '\n';
                }
        }
        s += "--";
        LOG(s);
}

template <std::size_t N>
void print_vertex_data(const std::vector<ManifoldVertex<N>>& vertices)
{
        std::string s;
        s += "-- vertices --\n";
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
                s += "pole " + to_string(i) + ": " + to_string(vertices[i].positive_norm);
                s += '\n';
        }
        s += "--";
        LOG(s);
}

#define TEMPLATE(N)                                                                                                    \
        template void print_delaunay_facets(const std::vector<core::DelaunayFacet<(N)>>&);                             \
        template void print_cocone_facets(const std::vector<core::DelaunayFacet<(N)>>&, const std::vector<bool>&);     \
        template void print_non_cocone_facets(const std::vector<core::DelaunayFacet<(N)>>&, const std::vector<bool>&); \
        template void print_vertex_data(const std::vector<ManifoldVertex<(N)>>&);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
