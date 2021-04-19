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
#include <src/geometry/spatial/shape_wrapper.h>
#include <src/numerical/ray.h>

#include <functional>
#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T>
struct Shape
{
        virtual ~Shape() = default;

        virtual std::optional<T> intersect_bounding(const Ray<N, T>& ray) const = 0;

        virtual const Intersection<N, T>* intersect(const Ray<N, T>& ray, T bounding_distance) const = 0;

        virtual geometry::BoundingBox<N, T> bounding_box() const = 0;

        virtual std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>
                intersection_function() const = 0;
};
}
