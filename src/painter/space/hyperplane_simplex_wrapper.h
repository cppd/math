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
template <typename Simplex, typename = void>
class HyperplaneSimplexWrapperForShapeIntersection final
{
        // Для меньшего количества измерений есть второй класс
        static_assert(Simplex::SPACE_DIMENSION >= 4);

        static constexpr size_t N = Simplex::SPACE_DIMENSION;
        using T = typename Simplex::DataType;

        static constexpr int VERTEX_COUNT = N;

        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        using Constraints = std::array<Constraint<N, T>, N>;
        using ConstraintsEq = std::array<Constraint<N, T>, 1>;

        const Simplex& m_simplex;

        Vertices m_vertices;
        Constraints m_constraints;
        ConstraintsEq m_constraints_eq;
        Vector<N, T> m_min, m_max;

public:
        static constexpr size_t SPACE_DIMENSION = Simplex::SPACE_DIMENSION;
        static constexpr size_t SHAPE_DIMENSION = Simplex::SHAPE_DIMENSION;
        using DataType = T;

        explicit HyperplaneSimplexWrapperForShapeIntersection(const Simplex& s) : m_simplex(s), m_vertices(s.vertices())
        {
                static_assert(std::remove_reference_t<decltype(s.vertices())>().size() == N);

                m_simplex.constraints(&m_constraints, &m_constraints_eq);

                min_max_vector(m_vertices, &m_min, &m_max);
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

template <typename Simplex>
class HyperplaneSimplexWrapperForShapeIntersection<Simplex, std::enable_if_t<Simplex::SPACE_DIMENSION == 3>> final
{
        static_assert(Simplex::SPACE_DIMENSION == 3);

        static constexpr size_t N = Simplex::SPACE_DIMENSION;
        using T = typename Simplex::DataType;

        using Vertices = std::array<Vector<N, T>, Simplex::VERTEX_COUNT>;

        // Элементы массива — вершина откуда и вектор к другой вершине
        using VertexRidges = std::array<std::array<Vector<N, T>, 2>, Simplex::VERTEX_RIDGE_COUNT>;

        const Simplex& m_simplex;

        Vertices m_vertices;
        VertexRidges m_vertex_ridges;

public:
        static constexpr size_t SPACE_DIMENSION = Simplex::SPACE_DIMENSION;
        static constexpr size_t SHAPE_DIMENSION = Simplex::SHAPE_DIMENSION;
        using DataType = T;

        explicit HyperplaneSimplexWrapperForShapeIntersection(const Simplex& s)
                : m_simplex(s), m_vertices(s.vertices()), m_vertex_ridges(s.vertex_ridges())
        {
        }

        bool intersect(const Ray<N, T>& r, T* t) const
        {
                return m_simplex.intersect(r, t);
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
