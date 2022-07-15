/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <tuple>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
struct Shape
{
        static_assert(std::is_floating_point_v<T>);

        virtual ~Shape() = default;

        [[nodiscard]] virtual T intersection_cost() const = 0;

        [[nodiscard]] virtual std::optional<T> intersect_bounds(const Ray<N, T>& ray, T max_distance) const = 0;

        [[nodiscard]] virtual std::tuple<T, const Surface<N, T, Color>*> intersect(
                const Ray<N, T>& ray,
                T max_distance,
                T bounding_distance) const = 0;

        [[nodiscard]] virtual bool intersect_any(const Ray<N, T>& ray, T max_distance, T bounding_distance) const = 0;

        [[nodiscard]] virtual geometry::BoundingBox<N, T> bounding_box() const = 0;

        [[nodiscard]] virtual std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)>
                overlap_function() const = 0;
};
}
