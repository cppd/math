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

#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/simplex.h>
#include <src/numerical/vec.h>

#include <array>
#include <utility>

namespace painter
{
namespace shape_intersection_implementation
{
#if 0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
template <typename Shape>
class HasInsideFunction
{
        using V = Vector<Shape::SPACE_DIMENSION, typename Shape::DataType>;

        template <typename T>
        static decltype(std::declval<T>().inside(V()), std::true_type()) t(int);
        template <typename>
        static std::false_type t(...);

public:
        static constexpr bool value = std::is_same_v<decltype(t<Shape>(0)), std::true_type>;
};
#pragma GCC diagnostic pop
#endif

template <typename T>
constexpr size_t size()
{
        return std::remove_reference_t<T>().size();
}

template <typename Shape1, typename Shape2>
bool shapes_intersect_by_vertices(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr size_t N = Shape1::SPACE_DIMENSION;
        using T = typename Shape1::DataType;

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

template <size_t N, typename T, typename Shape>
bool line_segment_intersects_shape(const Vector<N, T>& org, const Vector<N, T>& direction, const Shape& shape)
{
        static_assert(N == Shape::SPACE_DIMENSION);
        static_assert(std::is_same_v<T, typename Shape::DataType>);

        Ray<N, T> r(org, direction);
        T alpha;
        return shape.intersect(r, &alpha) && (square(alpha) < dot(direction, direction));
}

template <typename Shape1, typename Shape2>
bool shapes_intersect_by_vertex_ridges(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr size_t N = Shape1::SPACE_DIMENSION;
        using T = typename Shape1::DataType;

        for (const std::array<Vector<N, T>, 2>& ridge : shape_1.vertex_ridges())
        {
                if (line_segment_intersects_shape(ridge[0], ridge[1], shape_2))
                {
                        return true;
                }
        }

        for (const std::array<Vector<N, T>, 2>& ridge : shape_2.vertex_ridges())
        {
                if (line_segment_intersects_shape(ridge[0], ridge[1], shape_1))
                {
                        return true;
                }
        }

        return false;
}

template <size_t N, size_t V, typename T>
bool all_vertices_are_on_negative_side(const std::array<Vector<N, T>, V>& vertices, const Constraint<N, T>& c)
{
        for (const Vector<N, T>& v : vertices)
        {
                if (dot(v, c.a) + c.b > 0)
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, size_t V, typename T>
bool all_vertices_are_on_the_same_side(const std::array<Vector<N, T>, V>& vertices, const Constraint<N, T>& c)
{
        bool negative = false;
        bool positive = false;
        for (const Vector<N, T>& v : vertices)
        {
                T p = dot(v, c.a) + c.b;
                if ((p > 0 && negative) || (p < 0 && positive))
                {
                        return false;
                }
                negative = p < 0;
                positive = p > 0;
        }
        return true;
}

template <typename Shape1, typename Shape2>
bool shapes_not_intersect_by_planes(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr size_t N = Shape1::SPACE_DIMENSION;
        using T = typename Shape1::DataType;

        for (const Constraint<N, T>& constraint : shape_1.constraints())
        {
                if (all_vertices_are_on_negative_side(shape_2.vertices(), constraint))
                {
                        return true;
                }
        }

        for (const Constraint<N, T>& constraint : shape_2.constraints())
        {
                if (all_vertices_are_on_negative_side(shape_1.vertices(), constraint))
                {
                        return true;
                }
        }

        for (const Constraint<N, T>& constraint : shape_1.constraints_eq())
        {
                if (all_vertices_are_on_the_same_side(shape_2.vertices(), constraint))
                {
                        return true;
                }
        }

        for (const Constraint<N, T>& constraint : shape_2.constraints_eq())
        {
                if (all_vertices_are_on_the_same_side(shape_1.vertices(), constraint))
                {
                        return true;
                }
        }

        return false;
}

template <typename Shape1, typename Shape2>
bool shapes_intersect_by_spaces(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr size_t N = Shape1::SPACE_DIMENSION;
        using T = typename Shape1::DataType;

        constexpr size_t CONSTRAINT_COUNT =
                size<decltype(shape_1.constraints())>() + size<decltype(shape_2.constraints())>()
                + size<decltype(shape_1.constraints_eq())>() + size<decltype(shape_2.constraints_eq())>();

        const Vector<N, T> min = min_vector(shape_1.min(), shape_2.min());

        std::array<Vector<N, T>, CONSTRAINT_COUNT> a;
        std::array<T, CONSTRAINT_COUNT> b;

        // Со смещением минимума к нулю для всех ограничений,
        // чтобы работа была с положительными числами
        // x_new = x_old - min
        // x_old = x_new + min
        // a ⋅ (x_new + min) + b  ->  a ⋅ x_new + a ⋅ min + b  ->  a ⋅ x_new + (a ⋅ min + b)

        int i = 0;

        for (const Constraint<N, T>& c : shape_1.constraints())
        {
                a[i] = c.a;
                b[i] = dot(c.a, min) + c.b;
                ++i;
        }
        for (const Constraint<N, T>& c : shape_2.constraints())
        {
                a[i] = c.a;
                b[i] = dot(c.a, min) + c.b;
                ++i;
        }

        static_assert(1 >= size<decltype(shape_1.constraints_eq())>() + size<decltype(shape_2.constraints_eq())>());

        if constexpr (0 == size<decltype(shape_1.constraints_eq())>() + size<decltype(shape_2.constraints_eq())>())
        {
                ASSERT(i == CONSTRAINT_COUNT);

                return (numerical::solve_constraints(a, b) == numerical::ConstraintSolution::Feasible);
        }

        if constexpr (1 == size<decltype(shape_1.constraints_eq())>() + size<decltype(shape_2.constraints_eq())>())
        {
                ASSERT(i + 1 == CONSTRAINT_COUNT);

                const Constraint<N, T>& c =
                        !shape_1.constraints_eq().empty() ? shape_1.constraints_eq()[0] : shape_2.constraints_eq()[0];

                const Vector<N, T> a_v = c.a;
                const T b_v = dot(c.a, min) + c.b;

                a[i] = a_v;
                b[i] = b_v;
                if (numerical::solve_constraints(a, b) != numerical::ConstraintSolution::Feasible)
                {
                        return false;
                }
                a[i] = -a_v;
                b[i] = -b_v;
                return (numerical::solve_constraints(a, b) == numerical::ConstraintSolution::Feasible);
        }
}

template <typename Shape1, typename Shape2>
void static_checks(const Shape1& shape_1, const Shape2& shape_2)
{
        static_assert(Shape1::SPACE_DIMENSION == Shape2::SPACE_DIMENSION);
        static_assert(std::is_same_v<typename Shape1::DataType, typename Shape2::DataType>);

        constexpr size_t N = Shape1::SPACE_DIMENSION;

        static_assert(Shape1::SHAPE_DIMENSION == N || Shape1::SHAPE_DIMENSION + 1 == N);
        static_assert(Shape2::SHAPE_DIMENSION == N || Shape2::SHAPE_DIMENSION + 1 == N);

        if constexpr (N >= 4)
        {
                static_assert(size<decltype(shape_1.constraints())>() >= Shape1::SHAPE_DIMENSION + 1);
                static_assert(size<decltype(shape_2.constraints())>() >= Shape2::SHAPE_DIMENSION + 1);
                static_assert(size<decltype(shape_1.constraints_eq())>() + Shape1::SHAPE_DIMENSION == N);
                static_assert(size<decltype(shape_2.constraints_eq())>() + Shape2::SHAPE_DIMENSION == N);
        }
}
}

// Пересечение выпуклых объектов.
// * Достаточное условие пересечения:
//     Любая вершина одного объекта находится внутри другого объекта.
// * Достаточное условие отсутствия пересечения:
//     Все вершины одного объекта находятся по одну сторону от другого объекта.
// * Необходимое и достаточное условие пересечения (по определению пересечения):
//     Система неравенств объектов имеет решение.
//
//   Два достаточных условия используются для ускорения поиска пересечения,
// чтобы реже решать систему неравенств.
//
//   Для двухмерных и трёхмерных пространств можно обойтись без поиска решения
// системы неравенств. Объекты в трёхмерном пересекаются, если любая вершина
// одного объекта находится внутри другого объекта или ребро одного объекта
// пересекает другой объект. За исключением частных случаев, когда, например,
// объекты совпадают, но здесь эти случаи не учитываются.
template <typename Shape1, typename Shape2>
bool shape_intersection(const Shape1& shape_1, const Shape2& shape_2)
{
        namespace impl = shape_intersection_implementation;

        impl::static_checks(shape_1, shape_2);

        constexpr size_t N = Shape1::SPACE_DIMENSION;

        if (impl::shapes_intersect_by_vertices(shape_1, shape_2))
        {
                return true;
        }

        if constexpr (N <= 3)
        {
                return (impl::shapes_intersect_by_vertex_ridges(shape_1, shape_2));
        }

        if constexpr (N >= 4)
        {
                if (impl::shapes_not_intersect_by_planes(shape_1, shape_2))
                {
                        return false;
                }

                return (impl::shapes_intersect_by_spaces(shape_1, shape_2));
        }
}
}
