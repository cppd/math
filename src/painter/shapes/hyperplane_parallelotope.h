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

#include "shape.h"

#include "../objects.h"

#include <src/com/memory_arena.h>
#include <src/geometry/spatial/hyperplane_parallelotope.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/shading/ggx_diffuse.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class HyperplaneParallelotope final : public Shape<N, T, Color>
{
        const geometry::HyperplaneParallelotope<N, T> hyperplane_parallelotope_;
        std::optional<Color> light_source_;
        const T metalness_;
        const T roughness_;
        const Color color_;
        const T alpha_;
        const bool alpha_nonzero_ = alpha_ > 0;

        class IntersectionImpl final : public Surface<N, T, Color>
        {
                const HyperplaneParallelotope* obj_;

        public:
                IntersectionImpl(const Vector<N, T>& point, const HyperplaneParallelotope* obj)
                        : Surface<N, T, Color>(point), obj_(obj)
                {
                }

                Vector<N, T> geometric_normal() const override
                {
                        return obj_->hyperplane_parallelotope_.normal(this->point());
                }

                std::optional<Vector<N, T>> shading_normal() const override
                {
                        return std::nullopt;
                }

                std::optional<Color> light_source() const override
                {
                        return obj_->light_source_;
                }

                Color brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
                {
                        return shading::ggx_diffuse::f(obj_->metalness_, obj_->roughness_, obj_->color_, n, v, l);
                }

                shading::Sample<N, T, Color> sample_brdf(
                        RandomEngine<T>& random_engine,
                        const Vector<N, T>& n,
                        const Vector<N, T>& v) const override
                {
                        return shading::ggx_diffuse::sample_f(
                                random_engine, obj_->metalness_, obj_->roughness_, obj_->color_, n, v);
                }
        };

public:
        template <typename... V>
        HyperplaneParallelotope(
                const T metalness,
                const T roughness,
                const Color& color,
                const T alpha,
                const Vector<N, T>& org,
                const V&... e)
                : hyperplane_parallelotope_(org, e...),
                  metalness_(std::clamp(metalness, T(0), T(1))),
                  roughness_(std::clamp(roughness, T(0), T(1))),
                  color_(color.clamp(0, 1)),
                  alpha_(std::clamp(alpha, T(0), T(1)))
        {
        }

        void set_light_source(const Color& color)
        {
                light_source_ = color;
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override
        {
                if (alpha_nonzero_ || light_source_)
                {
                        return hyperplane_parallelotope_.intersect(r);
                }
                return std::nullopt;
        }

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray, const T bounding_distance) const override
        {
                return make_arena_ptr<IntersectionImpl>(ray.point(bounding_distance), this);
        }

        geometry::BoundingBox<N, T> bounding_box() const override
        {
                return geometry::BoundingBox<N, T>(hyperplane_parallelotope_.vertices());
        }

        std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>
                intersection_function() const override
        {
                return [w = geometry::ShapeWrapperForIntersection(hyperplane_parallelotope_)](
                               const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>& p)
                {
                        return geometry::shape_intersection(w, p);
                };
        }
};
}
