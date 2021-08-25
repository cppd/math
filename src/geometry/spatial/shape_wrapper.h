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

#include <optional>
#include <utility>

namespace ns::geometry
{
template <typename Shape>
class ShapeWrapperForIntersection final
{
        static_assert(Shape::SPACE_DIMENSION >= 4);

        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = typename Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using Constraints = decltype(std::declval<Shape>().constraints());

        const Shape& shape_;
        Vertices vertices_;
        Constraints constraints_;
        //Vector<N, T> min_;

        //template <std::size_t ArraySize, typename T, std::size_t N>
        //static Vector<N, T> find_min_vector(const std::array<Vector<N, T>, ArraySize>& vectors)
        //{
        //        static_assert(ArraySize > 0);
        //        Vector<N, T> min = vectors[0];
        //        for (std::size_t i = 1; i < ArraySize; ++i)
        //        {
        //                min = ::ns::min(vectors[i], min);
        //        }
        //        return min;
        //}

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeWrapperForIntersection(const Shape& s)
                : shape_(s), vertices_(s.vertices()), constraints_(shape_.constraints())
        {
                //min_ = find_min_vector(vertices_);
        }

        bool inside(const Vector<N, T>& p) const
        {
                return shape_.inside(p);
        }

        const Vertices& vertices() const
        {
                return vertices_;
        }

        const Constraints& constraints() const
        {
                return constraints_;
        }

        //const Vector<N, T>& min() const
        //{
        //        return min_;
        //}
};

template <typename Shape>
requires(Shape::SPACE_DIMENSION == 3 || Shape::SPACE_DIMENSION == 2) class ShapeWrapperForIntersection<Shape> final
{
        static constexpr std::size_t N = Shape::SPACE_DIMENSION;

        using T = typename Shape::DataType;
        using Vertices = decltype(std::declval<Shape>().vertices());
        using Edges = decltype(std::declval<Shape>().edges());

        const Shape& shape_;
        Vertices vertices_;
        Edges edges_;

public:
        static constexpr std::size_t SPACE_DIMENSION = Shape::SPACE_DIMENSION;
        static constexpr std::size_t SHAPE_DIMENSION = Shape::SHAPE_DIMENSION;
        using DataType = T;

        explicit ShapeWrapperForIntersection(const Shape& s) : shape_(s), vertices_(s.vertices()), edges_(s.edges())
        {
        }

        bool inside(const Vector<N, T>& p) const
        {
                return shape_.inside(p);
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return shape_.intersect(r);
        }

        const Vertices& vertices() const
        {
                return vertices_;
        }

        const Edges& edges() const
        {
                return edges_;
        }
};
}
