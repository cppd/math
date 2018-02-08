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

/*
 R. Stuart Ferguson.
 Practical Algorithms For 3D Computer Graphics, Second Edition.
 CRC Press, 2014.

 В частности, раздел 5.3.4 Octree decomposition.
*/

#pragma once

#include "com/math.h"
#include "com/ray.h"
#include "com/vec.h"

#include <array>
#include <utility>

namespace ShapeIntersectionImplementation
{
template <size_t N, typename T, typename Shape>
bool line_segment_intersects_shape(const Vector<N, T>& org, const Vector<N, T>& direction, const Shape& shape)
{
        static_assert(N == Shape::DIMENSION);
        static_assert(std::is_same_v<T, typename Shape::DataType>);

        Ray<N, T> r(org, direction);
        T alpha;
        return shape.intersect(r, &alpha) && (square(alpha) < dot(direction, direction));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
template <typename Shape>
class HasInsideFunction
{
        using V = Vector<Shape::DIMENSION, typename Shape::DataType>;
        template <typename T>
        static decltype(std::declval<T>().inside(V()), std::true_type()) t(int);
        template <typename>
        static std::false_type t(...);

public:
        static constexpr bool value = std::is_same_v<decltype(t<Shape>(0)), std::true_type>;
};
#pragma GCC diagnostic pop

template <typename Shape1, typename Shape2>
bool shapes_intersect_by_vertices(const Shape1& shape_1, const Shape2& shape_2)
{
        static_assert(HasInsideFunction<Shape1>::value == (Shape1::DIMENSION == Shape1::SHAPE_DIMENSION));
        static_assert(HasInsideFunction<Shape2>::value == (Shape2::DIMENSION == Shape2::SHAPE_DIMENSION));

        constexpr size_t N = Shape1::DIMENSION;
        using T = typename Shape1::DataType;

        if constexpr (HasInsideFunction<Shape2>::value)
        {
                for (const Vector<N, T>& v : shape_1.vertices())
                {
                        if (shape_2.inside(v))
                        {
                                return true;
                        }
                }
        }

        if constexpr (HasInsideFunction<Shape1>::value)
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

template <typename Shape1, typename Shape2>
bool shapes_intersect_by_vertex_ridges(const Shape1& shape_1, const Shape2& shape_2)
{
        constexpr size_t N = Shape1::DIMENSION;
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
}

// Два объекта пересекаются, если выполняется любое из условий:
//   1) какая-нибудь вершина одного объекта находится внутри другого объекта,
//   2) какое-нибудь линейное ребро одного объекта пересекает другой объект.
template <typename Shape1, typename Shape2>
bool shape_intersection(const Shape1& shape_1, const Shape2& shape_2)
{
        static_assert(Shape1::DIMENSION == Shape2::DIMENSION);
        static_assert(std::is_same_v<typename Shape1::DataType, typename Shape2::DataType>);
        static_assert(Shape1::DIMENSION >= Shape1::SHAPE_DIMENSION && Shape1::DIMENSION - Shape1::SHAPE_DIMENSION <= 1);
        static_assert(Shape2::DIMENSION >= Shape2::SHAPE_DIMENSION && Shape2::DIMENSION - Shape2::SHAPE_DIMENSION <= 1);

        if (ShapeIntersectionImplementation::shapes_intersect_by_vertices(shape_1, shape_2))
        {
                return true;
        }

        if (ShapeIntersectionImplementation::shapes_intersect_by_vertex_ridges(shape_1, shape_2))
        {
                return true;
        }

        return false;
}
