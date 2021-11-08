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
#include <src/geometry/spatial/parallelotope.h>
#include <src/shading/ggx_diffuse.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class Parallelotope final : public Shape<N, T, Color>
{
        const geometry::Parallelotope<N, T> parallelotope_;
        std::optional<Color> light_source_;
        const T metalness_;
        const T roughness_;
        const Color color_;
        const T alpha_;
        const bool alpha_nonzero_ = alpha_ > 0;

        //

        class SurfaceImpl final : public Surface<N, T, Color>
        {
                const Parallelotope* obj_;

                Vector<N, T> geometric_normal() const override
                {
                        return obj_->parallelotope_.normal(this->point());
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

                T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
                {
                        return shading::ggx_diffuse::pdf(obj_->roughness_, n, v, l);
                }

                Sample<N, T, Color> sample_brdf(
                        RandomEngine<T>& random_engine,
                        const Vector<N, T>& n,
                        const Vector<N, T>& v) const override
                {
                        shading::Sample<N, T, Color> sample = shading::ggx_diffuse::sample_f(
                                random_engine, obj_->metalness_, obj_->roughness_, obj_->color_, n, v);

                        Sample<N, T, Color> s;
                        s.l = sample.l;
                        s.pdf = sample.pdf;
                        s.brdf = sample.brdf;
                        s.specular = false;
                        return s;
                }

        public:
                SurfaceImpl(const Vector<N, T>& point, const Parallelotope* obj)
                        : Surface<N, T, Color>(point), obj_(obj)
                {
                }
        };

        //

        std::optional<T> intersect_bounding(const Ray<N, T>& r, const T max_distance) const override
        {
                if (alpha_nonzero_ || light_source_)
                {
                        std::optional<T> res = parallelotope_.intersect(r);
                        if (res && *res < max_distance)
                        {
                                return res;
                        }
                }
                return std::nullopt;
        }

        ShapeIntersection<N, T, Color> intersect(
                const Ray<N, T>& ray,
                const T /*max_distance*/,
                const T bounding_distance) const override
        {
                ShapeIntersection<N, T, Color> intersection;
                intersection.distance = bounding_distance;
                intersection.surface = make_arena_ptr<SurfaceImpl>(ray.point(bounding_distance), this);
                return intersection;
        }

        geometry::BoundingBox<N, T> bounding_box() const override
        {
                return geometry::BoundingBox<N, T>(parallelotope_.vertices());
        }

        std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)> overlap_function()
                const override
        {
                return parallelotope_.overlap_function();
        }

public:
        template <typename... V>
        Parallelotope(
                const T metalness,
                const T roughness,
                const Color& color,
                const T alpha,
                const Vector<N, T>& org,
                const V&... e)
                : parallelotope_(org, e...),
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
};
}
