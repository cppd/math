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

#include "../objects.h"

#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/numerical/ray.h>

#include <functional>
#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
struct ShapeIntersection final
{
        T distance;
        const Surface<N, T, Color>* surface;

        ShapeIntersection()
        {
        }

        explicit ShapeIntersection(std::nullptr_t) : surface(nullptr)
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct Shape
{
        virtual ~Shape() = default;

        virtual T intersection_cost() const = 0;

        virtual std::optional<T> intersect_bounding(const Ray<N, T>& ray, T max_distance) const = 0;

        virtual ShapeIntersection<N, T, Color> intersect(const Ray<N, T>& ray, T max_distance, T bounding_distance)
                const = 0;

        virtual geometry::BoundingBox<N, T> bounding_box() const = 0;

        virtual std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)> overlap_function()
                const = 0;
};
}
