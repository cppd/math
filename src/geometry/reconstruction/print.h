/*
Copyright (C) 2017-2021 Topological Manifold

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

#if 0

#include "structure.h"

#include "../core/delaunay.h"

#include <src/com/log.h>
#include <src/com/print.h>

#include <vector>

namespace ns::geometry
{
template <std::size_t N>
void print_delaunay_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets)
{
        LOG("--delaunay facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                LOG(to_string(delaunay_facets[i].vertices()));
        }
        LOG("--");
}

template <std::size_t N>
void print_cocone_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets)
{
        LOG("--cocone facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (cocone_facets[i])
                {
                        LOG(to_string(delaunay_facets[i].vertices()));
                }
        }
        LOG("--");
}

template <std::size_t N>
void print_not_cocone_facets(
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        LOG("--not cocone facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        LOG(to_string(delaunay_facets[i].vertices()));
                }
        }
        LOG("--");
}

template <std::size_t N>
void print_vertex_data(const std::vector<ManifoldVertex<N>>& vertices)
{
        LOG("--vertices--");
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
                LOG("pole " + to_string(i) + ": " + to_string(vertices[i].positive_norm));
        }
        LOG("--");
}
}

#endif
