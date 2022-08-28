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

#include "parallelotope.h"

#include <src/color/color.h>
#include <src/com/memory_arena.h>
#include <src/settings/instantiation.h>
#include <src/shading/ggx/brdf.h>
#include <src/shading/ggx/metalness.h>

namespace ns::painter::shapes
{
namespace
{
template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const Parallelotope<N, T, Color>* obj_;

        [[nodiscard]] Vector<N, T> point(const Ray<N, T>& ray, const T distance) const override
        {
                return obj_->parallelotope().project(ray.point(distance));
        }

        [[nodiscard]] Vector<N, T> geometric_normal(const Vector<N, T>& point) const override
        {
                return obj_->parallelotope().normal(point);
        }

        [[nodiscard]] std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& /*point*/) const override
        {
                return std::nullopt;
        }

        [[nodiscard]] std::optional<Color> light_source() const override
        {
                return obj_->light_source();
        }

        [[nodiscard]] Color brdf(
                const Vector<N, T>& /*point*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                return shading::ggx::brdf::f(obj_->roughness(), obj_->colors(), n, v, l);
        }

        [[nodiscard]] T pdf(
                const Vector<N, T>& /*point*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                return shading::ggx::brdf::pdf(obj_->roughness(), n, v, l);
        }

        [[nodiscard]] SurfaceSample<N, T, Color> sample(
                PCG& engine,
                const Vector<N, T>& /*point*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override
        {
                const shading::Sample<N, T, Color>& sample =
                        shading::ggx::brdf::sample_f(engine, obj_->roughness(), obj_->colors(), n, v);

                SurfaceSample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                return s;
        }

        [[nodiscard]] bool is_specular(const Vector<N, T>& /*point*/) const override
        {
                return false;
        }

public:
        explicit SurfaceImpl(const Parallelotope<N, T, Color>* const obj)
                : obj_(obj)
        {
        }
};

}

template <std::size_t N, typename T, typename Color>
T Parallelotope<N, T, Color>::intersection_cost() const
{
        return decltype(parallelotope_)::intersection_cost();
}

template <std::size_t N, typename T, typename Color>
std::optional<T> Parallelotope<N, T, Color>::intersect_bounds(const Ray<N, T>& r, const T max_distance) const
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

template <std::size_t N, typename T, typename Color>
ShapeIntersection<N, T, Color> Parallelotope<N, T, Color>::intersect(
        const Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T bounding_distance) const
{
        return {bounding_distance, make_arena_ptr<SurfaceImpl<N, T, Color>>(this)};
}

template <std::size_t N, typename T, typename Color>
bool Parallelotope<N, T, Color>::intersect_any(
        const Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T /*bounding_distance*/) const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
geometry::BoundingBox<N, T> Parallelotope<N, T, Color>::bounding_box() const
{
        return geometry::BoundingBox<N, T>(parallelotope_.vertices());
}

template <std::size_t N, typename T, typename Color>
std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)> Parallelotope<N, T, Color>::
        overlap_function() const
{
        return parallelotope_.overlap_function();
}

template <std::size_t N, typename T, typename Color>
Parallelotope<N, T, Color>::Parallelotope(
        const std::type_identity_t<T> metalness,
        const std::type_identity_t<T> roughness,
        const Color& color,
        const std::type_identity_t<T> alpha,
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N>& vectors)
        : parallelotope_(org, vectors),
          roughness_(std::clamp(roughness, T{0}, T{1})),
          colors_(shading::ggx::compute_metalness(color.clamp(0, 1), std::clamp(metalness, T{0}, T{1}))),
          alpha_(std::clamp(alpha, T{0}, T{1}))
{
}

template <std::size_t N, typename T, typename Color>
void Parallelotope<N, T, Color>::set_light_source(const Color& color)
{
        light_source_ = color;
}

template <std::size_t N, typename T, typename Color>
const geometry::Parallelotope<N, T>& Parallelotope<N, T, Color>::parallelotope() const
{
        return parallelotope_;
}

template <std::size_t N, typename T, typename Color>
const std::optional<Color>& Parallelotope<N, T, Color>::light_source() const
{
        return light_source_;
}

template <std::size_t N, typename T, typename Color>
T Parallelotope<N, T, Color>::roughness() const
{
        return roughness_;
}

template <std::size_t N, typename T, typename Color>
const shading::Colors<Color>& Parallelotope<N, T, Color>::colors() const
{
        return colors_;
}

#define TEMPLATE(N, T, C) template class Parallelotope<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
