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

#include "com/ray.h"
#include "com/vec.h"
#include "path_tracing/algorithm/algorithm.h"
#include "path_tracing/space/constraint.h"

#include <algorithm>
#include <array>
#include <type_traits>

template <typename Simplex, typename = void>
class HyperplaneSimplexWrapperForShapeIntersection
{
        static_assert(Simplex::SPACE_DIMENSION == Simplex::SHAPE_DIMENSION + 1);
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
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N - 1;
        using DataType = T;

        HyperplaneSimplexWrapperForShapeIntersection(const Simplex& s) : m_simplex(s), m_vertices(s.vertices())
        {
                static_assert(std::remove_reference_t<decltype(s.vertices())>().size() == N);

                m_simplex.constraints(&m_constraints, &m_constraints_eq[0]);

                vertex_min_max(m_vertices, &m_min, &m_max);
        }

        bool intersect(const Ray<N, T>& r, T* t) const
        {
                return m_simplex.intersect(r, t);
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
class HyperplaneSimplexWrapperForShapeIntersection<Simplex, std::enable_if_t<Simplex::SPACE_DIMENSION == 3>>
{
        static_assert(Simplex::SPACE_DIMENSION == Simplex::SHAPE_DIMENSION + 1);
        static_assert(Simplex::SPACE_DIMENSION == 3);

        static constexpr size_t N = Simplex::SPACE_DIMENSION;
        using T = typename Simplex::DataType;

        static constexpr int VERTEX_COUNT = N;

        // Количество сочетаний по 2 из N
        // N! / ((N - 2)! * 2!) = (N * (N - 1)) / 2
        static constexpr int VERTEX_RIDGE_COUNT = (N * (N - 1)) / 2;

        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        // Элементы массива — вершина откуда и вектор к другой вершине
        using VertexRidges = std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT>;

        const Simplex& m_simplex;

        Vertices m_vertices;
        VertexRidges m_vertex_ridges;

public:
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N - 1;
        using DataType = T;

        HyperplaneSimplexWrapperForShapeIntersection(const Simplex& s) : m_simplex(s), m_vertices(s.vertices())
        {
                static_assert(std::remove_reference_t<decltype(s.vertices())>().size() == N);

                unsigned n = 0;
                for (unsigned i = 0; i < m_vertices.size() - 1; ++i)
                {
                        for (unsigned j = i + 1; j < m_vertices.size(); ++j)
                        {
                                m_vertex_ridges[n++] = {{m_vertices[i], m_vertices[j] - m_vertices[i]}};
                        }
                }
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
