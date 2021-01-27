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

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <type_traits>
#include <utility>

namespace ns::geometry
{
template <typename Shape, typename = void>
class ShapeWrapperForIntersection final
{
        // Для меньшего количества измерений есть второй класс
        static_assert(Shape::SPACE_DIMENSION >= 4);

        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = typename Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using Constraints = decltype(std::declval<Shape>().constraints());

        const Shape& m_shape;
        Vertices m_vertices;
        Constraints m_constraints;
        Vector<N, T> m_min;

        template <std::size_t ArraySize, typename T, std::size_t N>
        static Vector<N, T> find_min_vector(const std::array<Vector<N, T>, ArraySize>& vectors)
        {
                static_assert(ArraySize > 0);
                Vector<N, T> min = vectors[0];
                for (std::size_t i = 1; i < ArraySize; ++i)
                {
                        min = min_vector(vectors[i], min);
                }
                return min;
        }

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeWrapperForIntersection(const Shape& s)
                : m_shape(s),
                  m_vertices(s.vertices()),
                  m_constraints(m_shape.constraints()),
                  m_min(find_min_vector(m_vertices))
        {
        }

        bool inside(const Vector<N, T>& p) const
        {
                return m_shape.inside(p);
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
};

template <typename Shape>
class ShapeWrapperForIntersection<Shape, std::enable_if_t<Shape::SPACE_DIMENSION == 3 || Shape::SPACE_DIMENSION == 2>>
        final
{
        static_assert(Shape::SPACE_DIMENSION == 3 || Shape::SPACE_DIMENSION == 2);

        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = typename Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using VertexRidges = decltype(std::declval<Shape>().vertex_ridges());

        const Shape& m_shape;
        Vertices m_vertices;
        VertexRidges m_vertex_ridges;

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeWrapperForIntersection(const Shape& s)
                : m_shape(s), m_vertices(s.vertices()), m_vertex_ridges(s.vertex_ridges())
        {
        }

        bool inside(const Vector<N, T>& p) const
        {
                return m_shape.inside(p);
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return m_shape.intersect(r);
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
