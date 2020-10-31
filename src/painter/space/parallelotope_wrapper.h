/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "constraint.h"

#include <src/numerical/algorithm.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <type_traits>

namespace painter
{
template <typename Parallelotope, typename = void>
class ParallelotopeWrapperForShapeIntersection final
{
        // Для меньшего количества измерений есть второй класс
        static_assert(Parallelotope::SPACE_DIMENSION >= 4);

        static constexpr size_t N = Parallelotope::SPACE_DIMENSION;

        using T = typename Parallelotope::DataType;
        using Vertices = decltype(std::declval<Parallelotope>().vertices());
        using Constraints = decltype(std::declval<Parallelotope>().constraints());

        const Parallelotope& m_parallelotope;

        Vertices m_vertices;
        Constraints m_constraints;
        Vector<N, T> m_min, m_max;

public:
        static constexpr size_t SPACE_DIMENSION = Parallelotope::SPACE_DIMENSION;
        static constexpr size_t SHAPE_DIMENSION = Parallelotope::SHAPE_DIMENSION;
        using DataType = T;

        explicit ParallelotopeWrapperForShapeIntersection(const Parallelotope& p)
                : m_parallelotope(p), m_vertices(p.vertices()), m_constraints(p.constraints())
        {
                min_max_vector(m_vertices, &m_min, &m_max);
        }

        bool inside(const Vector<N, T>& p) const
        {
                return m_parallelotope.inside(p);
        }

        const Vertices& vertices() const
        {
                return m_vertices;
        }

        const Constraints& constraints() const
        {
                return m_constraints;
        }

        const Vector<N, T>& min() const
        {
                return m_min;
        }

        const Vector<N, T>& max() const
        {
                return m_max;
        }
};

template <typename Parallelotope>
class ParallelotopeWrapperForShapeIntersection<
        Parallelotope,
        std::enable_if_t<Parallelotope::SPACE_DIMENSION == 2 || Parallelotope::SPACE_DIMENSION == 3>>
        final
{
        static_assert(Parallelotope::SPACE_DIMENSION == 2 || Parallelotope::SPACE_DIMENSION == 3);

        static constexpr size_t N = Parallelotope::SPACE_DIMENSION;

        using T = typename Parallelotope::DataType;
        using Vertices = decltype(std::declval<Parallelotope>().vertices());
        using VertexRidges = decltype(std::declval<Parallelotope>().vertex_ridges());

        const Parallelotope& m_parallelotope;

        Vertices m_vertices;
        VertexRidges m_vertex_ridges;

public:
        static constexpr size_t SPACE_DIMENSION = Parallelotope::SPACE_DIMENSION;
        static constexpr size_t SHAPE_DIMENSION = Parallelotope::SHAPE_DIMENSION;
        using DataType = T;

        explicit ParallelotopeWrapperForShapeIntersection(const Parallelotope& p)
                : m_parallelotope(p), m_vertices(p.vertices()), m_vertex_ridges(p.vertex_ridges())
        {
        }

        bool intersect(const Ray<N, T>& r, DataType* t) const
        {
                return m_parallelotope.intersect(r, t);
        }

        bool inside(const Vector<N, T>& p) const
        {
                return m_parallelotope.inside(p);
        }

        const Vertices& vertices() const
        {
                return m_vertices;
        }

        const VertexRidges& vertex_ridges() const
        {
                return m_vertex_ridges;
        }
};
}
