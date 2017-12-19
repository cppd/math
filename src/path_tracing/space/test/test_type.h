/*
Copyright (C) 2017 Topological Manifold

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

#include "path_tracing/space/parallelotope_algorithm.h"
#include "path_tracing/space/shape_intersection.h"

template <typename Parallelotope>
class ParallelotopeWithVerticesAndRidges : public IntersectionParallelotope<ParallelotopeWithVerticesAndRidges<Parallelotope>>
{
        using VertexRidges = typename ParallelotopeTraits<Parallelotope>::VertexRidges;
        using Vertices = typename ParallelotopeTraits<Parallelotope>::Vertices;

        const Parallelotope& m_parallelotope;

        VertexRidges m_vertex_ridges;
        Vertices m_vertices;

public:
        static constexpr size_t DIMENSION = Parallelotope::DIMENSION;
        using DataType = typename Parallelotope::DataType;

        ParallelotopeWithVerticesAndRidges(const Parallelotope& p)
                : m_parallelotope(p), m_vertex_ridges(::vertex_ridges(p)), m_vertices(::vertices(p))
        {
        }
        bool intersect(const Ray<DIMENSION, DataType>& r, DataType* t) const
        {
                return m_parallelotope.intersect(r, t);
        }
        bool inside(const Vector<DIMENSION, DataType>& p) const
        {
                return m_parallelotope.inside(p);
        }
        const VertexRidges& vertex_ridges() const
        {
                return m_vertex_ridges;
        }
        const Vertices& vertices() const
        {
                return m_vertices;
        }
};
