/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/exponent.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace ns::geometry::spatial
{
namespace shape_overlap_implementation
{
template <typename T>
constexpr std::size_t size()
{
        return std::tuple_size_v<T>;
}

template <typename Shape1, typename Shape2>
bool shapes_overlap_by_vertices(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr std::size_t N = Shape1::SPACE_DIMENSION;
        using T = Shape1::DataType;

        if constexpr (Shape2::SPACE_DIMENSION == Shape2::SHAPE_DIMENSION)
        {
                for (const Vector<N, T>& v : shape_1.vertices())
                {
                        if (shape_2.inside(v))
                        {
                                return true;
                        }
                }
        }

        if constexpr (Shape1::SPACE_DIMENSION == Shape1::SHAPE_DIMENSION)
        {
                for (const Vector<N, T>& v : shape_2.vertices())
                {
                        if (shape_1.inside(v))
                        {
                                return true;
                        }
                }
        }

        return false;
}

template <std::size_t N, typename T, typename Shape>
bool line_segment_intersects_shape(const Vector<N, T>& org, const Vector<N, T>& direction, const Shape& shape)
{
        static_assert(N == Shape::SPACE_DIMENSION);
        static_assert(std::is_same_v<T, typename Shape::DataType>);

        const Ray<N, T> r(org, direction);
        const std::optional<T> alpha = shape.intersect(r);
        return alpha && (square(*alpha) < dot(direction, direction));
}

template <typename Shape1, typename Shape2>
bool shapes_overlap_by_edges(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr std::size_t N = Shape1::SPACE_DIMENSION;
        using T = Shape1::DataType;

        for (const std::array<Vector<N, T>, 2>& edge : shape_1.edges())
        {
                if (line_segment_intersects_shape(edge[0], edge[1], shape_2))
                {
                        return true;
                }
        }

        for (const std::array<Vector<N, T>, 2>& edge : shape_2.edges())
        {
                if (line_segment_intersects_shape(edge[0], edge[1], shape_1))
                {
                        return true;
                }
        }

        return false;
}

template <std::size_t N, std::size_t V, typename T>
bool all_vertices_are_on_negative_side(const std::array<Vector<N, T>, V>& vertices, const Constraint<N, T>& c)
{
        for (const Vector<N, T>& v : vertices)
        {
                if (dot(v, c.a) + c.b >= 0)
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, std::size_t V, typename T>
bool all_vertices_are_only_on_one_side(const std::array<Vector<N, T>, V>& vertices, const Constraint<N, T>& c)
{
        bool non_positive = false;
        bool non_negative = false;
        for (const Vector<N, T>& v : vertices)
        {
                const T p = dot(v, c.a) + c.b;
                non_positive = non_positive || p <= 0;
                non_negative = non_negative || p >= 0;
                if (non_negative && non_positive)
                {
                        return false;
                }
        }
        return true;
}

template <typename Shape, typename ConstraintShape>
bool shape_is_on_negative_side(const Shape& shape, const ConstraintShape& constraint_shape)
{
        if (size<decltype(constraint_shape.constraints().c)>() > 0)
        {
                const auto& constraints = constraint_shape.constraints();
                const auto& vertices = shape.vertices();
                for (const auto& c : constraints.c)
                {
                        if (all_vertices_are_on_negative_side(vertices, c))
                        {
                                return true;
                        }
                }
        }
        return false;
}

template <typename Shape, typename ConstraintShape>
bool shape_is_on_one_side(const Shape& shape, const ConstraintShape& constraint_shape)
{
        if constexpr (ConstraintShape::SPACE_DIMENSION > ConstraintShape::SHAPE_DIMENSION)
        {
                static_assert(size<decltype(constraint_shape.constraints().c_eq)>() > 0);

                const auto& constraints = constraint_shape.constraints();
                const auto& vertices = shape.vertices();
                for (const auto& c_eq : constraints.c_eq)
                {
                        if (all_vertices_are_only_on_one_side(vertices, c_eq))
                        {
                                return true;
                        }
                }
        }
        return false;
}

template <typename Shape1, typename Shape2>
bool shapes_not_overlap_by_planes(const Shape1& shape_1, const Shape2& shape_2)
{
        if (shape_is_on_negative_side(shape_1, shape_2))
        {
                return true;
        }

        if (shape_is_on_negative_side(shape_2, shape_1))
        {
                return true;
        }

        if (shape_is_on_one_side(shape_1, shape_2))
        {
                return true;
        }

        if (shape_is_on_one_side(shape_2, shape_1))
        {
                return true;
        }

        return false;
}

// template <typename Shape>
//         requires (size<decltype(std::declval<Shape>().constraints().c_eq)>() >= 0)
// constexpr std::size_t constraint_count()
// {
//         static_assert(size<decltype(std::declval<Shape>().constraints().c)>() > 0);
//         static_assert(size<decltype(std::declval<Shape>().constraints().c_eq)>() > 0);
//         static_assert(
//                 Shape::SHAPE_DIMENSION + size<decltype(std::declval<Shape>().constraints().c_eq)>()
//                 == Shape::SPACE_DIMENSION);
//         return size<decltype(std::declval<Shape>().constraints().c)>()
//                + size<decltype(std::declval<Shape>().constraints().c_eq)>();
// }

// template <typename Shape>
//         requires (Shape::SPACE_DIMENSION == Shape::SHAPE_DIMENSION)
// constexpr std::size_t constraint_count()
// {
//         static_assert(size<decltype(std::declval<Shape>().constraints().c)>() > 0);
//         return size<decltype(std::declval<Shape>().constraints().c)>();
// }

// template <typename Shape1, typename Shape2>
// const Constraint<Shape1::SPACE_DIMENSION, typename Shape1::DataType>& constraint_eq(
//         const Shape1& shape_1,
//         const Shape2& shape_2)
// {
//         static_assert(
//                 (size<decltype(std::declval<Shape1>().constraints().c)>() == constraint_count<Shape1>())
//                 != (size<decltype(std::declval<Shape2>().constraints().c)>() == constraint_count<Shape2>()));
//         if constexpr (size<decltype(std::declval<Shape1>().constraints().c)>() == constraint_count<Shape1>())
//         {
//                 static_assert(std::is_reference_v<decltype(shape_2.constraints())>);
//                 return shape_2.constraints().c_eq[0];
//         }
//         if constexpr (size<decltype(std::declval<Shape2>().constraints().c)>() == constraint_count<Shape2>())
//         {
//                 static_assert(std::is_reference_v<decltype(shape_1.constraints())>);
//                 return shape_1.constraints().c_eq[0];
//         }
// }

// template <typename Shape1, typename Shape2>
// bool shapes_overlap_by_spaces(const Shape1& shape_1, const Shape2& shape_2)
// {
//         constexpr std::size_t N = Shape1::SPACE_DIMENSION;
//         using T = Shape1::DataType;
//
//         static_assert(size<decltype(shape_1.constraints().c)>() > 0);
//         static_assert(size<decltype(shape_2.constraints().c)>() > 0);
//
//         constexpr std::size_t CONSTRAINT_COUNT = constraint_count<Shape1>() + constraint_count<Shape2>();
//
//         const Vector<N, T> min = ::ns::min(shape_1.min(), shape_2.min());
//
//         std::array<Vector<N, T>, CONSTRAINT_COUNT> a;
//         std::array<T, CONSTRAINT_COUNT> b;
//
//         // make non-negative numbers
//         // x_new = x_old - min
//         // x_old = x_new + min
//         // a ⋅ (x_new + min) + b  ->  a ⋅ x_new + a ⋅ min + b  ->  a ⋅ x_new + (a ⋅ min + b)
//
//         int i = 0;
//
//         for (const Constraint<N, T>& c : shape_1.constraints().c)
//         {
//                 a[i] = c.a;
//                 b[i] = dot(c.a, min) + c.b;
//                 ++i;
//         }
//         for (const Constraint<N, T>& c : shape_2.constraints().c)
//         {
//                 a[i] = c.a;
//                 b[i] = dot(c.a, min) + c.b;
//                 ++i;
//         }
//
//         static_assert(
//                 CONSTRAINT_COUNT
//                 <= 1 + size<decltype(shape_1.constraints().c)>() + size<decltype(shape_2.constraints().c)>());
//
//         if constexpr (
//                 CONSTRAINT_COUNT
//                 == size<decltype(shape_1.constraints().c)>() + size<decltype(shape_2.constraints().c)>())
//         {
//                 ASSERT(i == CONSTRAINT_COUNT);
//
//                 return (numerical::solve_constraints(a, b) == numerical::ConstraintSolution::FEASIBLE);
//         }
//
//         if constexpr (
//                 CONSTRAINT_COUNT
//                 == 1 + size<decltype(shape_1.constraints().c)>() + size<decltype(shape_2.constraints().c)>())
//         {
//                 ASSERT(i + 1 == CONSTRAINT_COUNT);
//
//                 const Constraint<N, T>& c = constraint_eq(shape_1, shape_2);
//
//                 const Vector<N, T> a_v = c.a;
//                 const T b_v = dot(c.a, min) + c.b;
//
//                 a[i] = a_v;
//                 b[i] = b_v;
//                 if (numerical::solve_constraints(a, b) != numerical::ConstraintSolution::FEASIBLE)
//                 {
//                         return false;
//                 }
//                 a[i] = -a_v;
//                 b[i] = -b_v;
//                 return (numerical::solve_constraints(a, b) == numerical::ConstraintSolution::FEASIBLE);
//         }
// }

template <typename Shape1, typename Shape2>
void static_checks(const Shape1& shape_1, const Shape2& shape_2)
{
        static_assert(Shape1::SPACE_DIMENSION == Shape2::SPACE_DIMENSION);
        static_assert(std::is_same_v<typename Shape1::DataType, typename Shape2::DataType>);

        constexpr std::size_t N = Shape1::SPACE_DIMENSION;

        static_assert(Shape1::SHAPE_DIMENSION == N || Shape1::SHAPE_DIMENSION + 1 == N);
        static_assert(Shape2::SHAPE_DIMENSION == N || Shape2::SHAPE_DIMENSION + 1 == N);

        if constexpr (N >= 4)
        {
                static_assert(size<decltype(shape_1.constraints().c)>() >= Shape1::SHAPE_DIMENSION + 1);
                static_assert(size<decltype(shape_2.constraints().c)>() >= Shape2::SHAPE_DIMENSION + 1);
                if constexpr (N > Shape1::SHAPE_DIMENSION)
                {
                        static_assert(size<decltype(shape_1.constraints().c_eq)>() + Shape1::SHAPE_DIMENSION == N);
                }
                if constexpr (N > Shape2::SHAPE_DIMENSION)
                {
                        static_assert(size<decltype(shape_2.constraints().c_eq)>() + Shape2::SHAPE_DIMENSION == N);
                }
        }
}
}

template <typename Shape>
class ShapeOverlap final
{
        static_assert(Shape::SPACE_DIMENSION >= 4);

        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using Constraints = decltype(std::declval<Shape>().constraints());

        const Shape* shape_;
        Vertices vertices_;
        Constraints constraints_;
        // Vector<N, T> min_;

        // template <std::size_t ArraySize, typename T, std::size_t N>
        // static Vector<N, T> find_min_vector(const std::array<Vector<N, T>, ArraySize>& vectors)
        // {
        //         static_assert(ArraySize > 0);
        //         Vector<N, T> min = vectors[0];
        //         for (std::size_t i = 1; i < ArraySize; ++i)
        //         {
        //                 min = ::ns::min(vectors[i], min);
        //         }
        //         return min;
        // }

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeOverlap(const Shape* const shape)
                : shape_(shape),
                  vertices_(shape->vertices()),
                  constraints_(shape_->constraints())
        {
                // min_ = find_min_vector(vertices_);
        }

        [[nodiscard]] bool inside(const Vector<N, T>& p) const
        {
                return shape_->inside(p);
        }

        [[nodiscard]] const Vertices& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] const Constraints& constraints() const
        {
                return constraints_;
        }

        // [[nodiscard]] const Vector<N, T>& min() const
        // {
        //         return min_;
        // }
};

template <typename Shape>
        requires (Shape::SPACE_DIMENSION == 3 || Shape::SPACE_DIMENSION == 2)
class ShapeOverlap<Shape> final
{
        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using Edges = decltype(std::declval<Shape>().edges());

        const Shape* shape_;
        Vertices vertices_;
        Edges edges_;

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeOverlap(const Shape* const shape)
                : shape_(shape),
                  vertices_(shape->vertices()),
                  edges_(shape->edges())
        {
        }

        [[nodiscard]] bool inside(const Vector<N, T>& p) const
        {
                return shape_->inside(p);
        }

        [[nodiscard]] std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return shape_->intersect(r);
        }

        [[nodiscard]] const Vertices& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] const Edges& edges() const
        {
                return edges_;
        }
};

template <typename Shape1, typename Shape2>
[[nodiscard]] bool shapes_overlap(const ShapeOverlap<Shape1>& shape_1, const ShapeOverlap<Shape2>& shape_2)
{
        namespace impl = shape_overlap_implementation;

        impl::static_checks(shape_1, shape_2);

        constexpr std::size_t N = Shape1::SPACE_DIMENSION;

        if (impl::shapes_overlap_by_vertices(shape_1, shape_2))
        {
                return true;
        }

        if constexpr (N <= 3)
        {
                return impl::shapes_overlap_by_edges(shape_1, shape_2);
        }
        else
        {
                return !impl::shapes_not_overlap_by_planes(shape_1, shape_2);
                // if (impl::shapes_not_overlap_by_planes(shape_1, shape_2))
                // {
                //         return false;
                // }
                //
                // return (impl::shapes_overlap_by_spaces(shape_1, shape_2));
        }
}
}
