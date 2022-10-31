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

#include <src/geometry/spatial/parallelotope.h>
#include <src/shading/objects.h>

namespace ns::painter::shapes
{
template <std::size_t N, typename T, typename Color>
class Parallelotope final : public Shape<N, T, Color>
{
        const geometry::Parallelotope<N, T> parallelotope_;
        const T roughness_;
        const shading::Colors<Color> colors_;
        const T alpha_;
        const bool alpha_nonzero_ = alpha_ > 0;

        T intersection_cost() const override;

        std::optional<T> intersect_bounds(const Ray<N, T>& ray, T max_distance) const override;

        ShapeIntersection<N, T, Color> intersect(const Ray<N, T>& ray, T max_distance, T bounding_distance)
                const override;

        bool intersect_any(const Ray<N, T>& ray, T max_distance, T bounding_distance) const override;

        geometry::BoundingBox<N, T> bounding_box() const override;

        std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)> overlap_function()
                const override;

public:
        Parallelotope(
                std::type_identity_t<T> metalness,
                std::type_identity_t<T> roughness,
                const Color& color,
                std::type_identity_t<T> alpha,
                const Vector<N, T>& org,
                const std::array<Vector<N, T>, N>& vectors);

        const geometry::Parallelotope<N, T>& parallelotope() const;

        const LightSource<N, T, Color>* light_source() const;

        T roughness() const;

        const shading::Colors<Color>& colors() const;
};
}
