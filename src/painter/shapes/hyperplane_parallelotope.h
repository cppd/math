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

#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/hyperplane_parallelotope.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/shading/objects.h>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>

namespace ns::painter::shapes
{
template <std::size_t N, typename T, typename Color>
class HyperplaneParallelotope final : public Shape<N, T, Color>
{
        const geometry::spatial::HyperplaneParallelotope<N, T> hyperplane_parallelotope_;
        const T roughness_;
        const shading::Colors<Color> colors_;
        const T alpha_;
        const bool alpha_nonzero_ = alpha_ > 0;
        const LightSource<N, T, Color>* light_source_ = nullptr;

        [[nodiscard]] T intersection_cost() const override;

        [[nodiscard]] std::optional<T> intersect_bounds(const numerical::Ray<N, T>& ray, T max_distance) const override;

        [[nodiscard]] ShapeIntersection<N, T, Color> intersect(
                const numerical::Ray<N, T>& ray,
                T max_distance,
                T bounding_distance) const override;

        [[nodiscard]] bool intersect_any(const numerical::Ray<N, T>& ray, T max_distance, T bounding_distance)
                const override;

        [[nodiscard]] geometry::spatial::BoundingBox<N, T> bounding_box() const override;

        [[nodiscard]] std::function<
                bool(const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>&)>
                overlap_function() const override;

public:
        HyperplaneParallelotope(
                std::type_identity_t<T> metalness,
                std::type_identity_t<T> roughness,
                const Color& color,
                std::type_identity_t<T> alpha,
                const numerical::Vector<N, T>& org,
                const std::array<numerical::Vector<N, T>, N - 1>& vectors);

        [[nodiscard]] const geometry::spatial::HyperplaneParallelotope<N, T>& hyperplane_parallelotope() const;

        void set_light_source(const LightSource<N, T, Color>* light_source);
        [[nodiscard]] const LightSource<N, T, Color>* light_source() const;

        [[nodiscard]] T roughness() const;

        [[nodiscard]] const shading::Colors<Color>& colors() const;

        [[nodiscard]] T alpha() const;
};
}
