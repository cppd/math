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

#include "shape.h"

#include "../objects.h"

#include <src/com/memory_arena.h>
#include <src/geometry/spatial/parallelotope.h>
#include <src/shading/ggx_diffuse.h>
#include <src/shading/metalness.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class Parallelotope final : public Shape<N, T, Color>
{
        const geometry::Parallelotope<N, T> parallelotope_;
        std::optional<Color> light_source_;
        const T roughness_;
        const shading::Colors<Color> colors_;
        const T alpha_;
        const bool alpha_nonzero_ = alpha_ > 0;

        //

        class SurfaceImpl final : public Surface<N, T, Color>
        {
                const Parallelotope* obj_;

                Vector<N, T> point(const Ray<N, T>& ray, const T distance) const override
                {
                        return obj_->parallelotope_.project(ray.point(distance));
                }

                Vector<N, T> geometric_normal(const Vector<N, T>& point) const override
                {
                        return obj_->parallelotope_.normal(point);
                }

                std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& /*point*/) const override
                {
                        return std::nullopt;
                }

                std::optional<Color> light_source() const override
                {
                        return obj_->light_source_;
                }

                Color brdf(
                        const Vector<N, T>& /*point*/,
                        const Vector<N, T>& n,
                        const Vector<N, T>& v,
                        const Vector<N, T>& l) const override
                {
                        return shading::ggx_diffuse::f(obj_->roughness_, obj_->colors_, n, v, l);
                }

                T pdf(const Vector<N, T>& /*point*/,
                      const Vector<N, T>& n,
                      const Vector<N, T>& v,
                      const Vector<N, T>& l) const override
                {
                        return shading::ggx_diffuse::pdf(obj_->roughness_, n, v, l);
                }

                Sample<N, T, Color> sample_brdf(
                        PCG& engine,
                        const Vector<N, T>& /*point*/,
                        const Vector<N, T>& n,
                        const Vector<N, T>& v) const override
                {
                        const shading::Sample<N, T, Color>& sample =
                                shading::ggx_diffuse::sample_f(engine, obj_->roughness_, obj_->colors_, n, v);

                        Sample<N, T, Color> s;
                        s.l = sample.l;
                        s.pdf = sample.pdf;
                        s.brdf = sample.brdf;
                        s.specular = false;
                        return s;
                }

        public:
                explicit SurfaceImpl(const Parallelotope* const obj)
                        : obj_(obj)
                {
                }
        };

        //

        T intersection_cost() const override
        {
                return decltype(parallelotope_)::intersection_cost();
        }

        std::optional<T> intersect_bounds(const Ray<N, T>& r, const T max_distance) const override
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

        std::tuple<T, const Surface<N, T, Color>*> intersect(
                const Ray<N, T>& /*ray*/,
                const T /*max_distance*/,
                const T bounding_distance) const override
        {
                return {bounding_distance, make_arena_ptr<SurfaceImpl>(this)};
        }

        bool intersect_any(const Ray<N, T>& /*ray*/, const T /*max_distance*/, const T /*bounding_distance*/)
                const override
        {
                return true;
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
                  roughness_(std::clamp(roughness, T{0}, T{1})),
                  colors_(shading::compute_metalness(color.clamp(0, 1), std::clamp(metalness, T{0}, T{1}))),
                  alpha_(std::clamp(alpha, T{0}, T{1}))
        {
        }

        void set_light_source(const Color& color)
        {
                light_source_ = color;
        }
};
}
