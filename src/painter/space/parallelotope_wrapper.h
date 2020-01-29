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
#include "parallelotope_algorithm.h"

#include "../algorithm/algorithm.h"

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <type_traits>

template <typename Parallelotope, typename = void>
class ParallelotopeWrapperForShapeIntersection
{
        static constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        using Vertices = typename ParallelotopeAlgorithm<Parallelotope>::Vertices;
        using Constraints = std::array<Constraint<N, T>, 2 * N>;
        using ConstraintsEq = std::array<Constraint<N, T>, 0>;

        const Parallelotope& m_parallelotope;

        Vertices m_vertices;
        Constraints m_constraints;
        static constexpr ConstraintsEq m_constraints_eq{};

        Vector<N, T> m_min, m_max;

public:
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N;
        using DataType = T;

        explicit ParallelotopeWrapperForShapeIntersection(const Parallelotope& p)
                : m_parallelotope(p), m_vertices(parallelotope_vertices(p))
        {
                m_parallelotope.constraints(&m_constraints);

                vertex_min_max(m_vertices, &m_min, &m_max);
        }

        bool intersect(const Ray<N, T>& r, T* t) const
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

        const Constraints& constraints() const
        {
                return m_constraints;
        }

        const ConstraintsEq& constraints_eq() const
        {
                return m_constraints_eq;
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
        std::enable_if_t<Parallelotope::DIMENSION == 2 || Parallelotope::DIMENSION == 3>>
{
        static constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        using VertexRidges = typename ParallelotopeAlgorithm<Parallelotope>::VertexRidges;
        using Vertices = typename ParallelotopeAlgorithm<Parallelotope>::Vertices;

        const Parallelotope& m_parallelotope;

        Vertices m_vertices;
        VertexRidges m_vertex_ridges;

public:
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N;
        using DataType = T;

        explicit ParallelotopeWrapperForShapeIntersection(const Parallelotope& p)
                : m_parallelotope(p),
                  m_vertices(parallelotope_vertices(p)),
                  m_vertex_ridges(parallelotope_vertex_ridges(p))
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
