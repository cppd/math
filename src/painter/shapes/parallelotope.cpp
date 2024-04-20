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

#include "parallelotope.h"

#include <src/com/memory_arena.h>
#include <src/com/random/pcg.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/settings/instantiation.h>
#include <src/shading/ggx/brdf.h>
#include <src/shading/ggx/metalness.h>
#include <src/shading/objects.h>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>

namespace ns::painter::shapes
{
namespace
{
template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const Parallelotope<N, T, Color>* obj_;

        [[nodiscard]] numerical::Vector<N, T> point(const numerical::Ray<N, T>& ray, const T distance) const override
        {
                return obj_->parallelotope().project(ray.point(distance));
        }

        [[nodiscard]] numerical::Vector<N, T> geometric_normal(const numerical::Vector<N, T>& point) const override
        {
                return obj_->parallelotope().normal(point);
        }

        [[nodiscard]] std::optional<numerical::Vector<N, T>> shading_normal(
                const numerical::Vector<N, T>& /*point*/) const override
        {
                return std::nullopt;
        }

        [[nodiscard]] const LightSource<N, T, Color>* light_source() const override
        {
                return obj_->light_source();
        }

        [[nodiscard]] Color brdf(
                const numerical::Vector<N, T>& /*point*/,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                return shading::ggx::brdf::f(obj_->roughness(), obj_->colors(), n, v, l);
        }

        [[nodiscard]] T pdf(
                const numerical::Vector<N, T>& /*point*/,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                return shading::ggx::brdf::pdf(obj_->roughness(), n, v, l);
        }

        [[nodiscard]] SurfaceSample<N, T, Color> sample(
                PCG& engine,
                const numerical::Vector<N, T>& /*point*/,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v) const override
        {
                const shading::Sample<N, T, Color>& sample =
                        shading::ggx::brdf::sample_f(engine, obj_->roughness(), obj_->colors(), n, v);

                SurfaceSample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                return s;
        }

        [[nodiscard]] bool is_specular(const numerical::Vector<N, T>& /*point*/) const override
        {
                return false;
        }

        [[nodiscard]] T alpha(const numerical::Vector<N, T>& /*point*/) const override
        {
                return obj_->alpha();
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
std::optional<T> Parallelotope<N, T, Color>::intersect_bounds(const numerical::Ray<N, T>& ray, const T max_distance)
        const
{
        if (alpha_nonzero_)
        {
                std::optional<T> res = parallelotope_.intersect_volume(ray, max_distance);
                if (res && *res < max_distance)
                {
                        return res;
                }
        }
        return std::nullopt;
}

template <std::size_t N, typename T, typename Color>
ShapeIntersection<N, T, Color> Parallelotope<N, T, Color>::intersect(
        const numerical::Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T bounding_distance) const
{
        return {bounding_distance, make_arena_ptr<SurfaceImpl<N, T, Color>>(this)};
}

template <std::size_t N, typename T, typename Color>
bool Parallelotope<N, T, Color>::intersect_any(
        const numerical::Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T /*bounding_distance*/) const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
geometry::spatial::BoundingBox<N, T> Parallelotope<N, T, Color>::bounding_box() const
{
        return geometry::spatial::BoundingBox<N, T>(parallelotope_.vertices());
}

template <std::size_t N, typename T, typename Color>
std::function<bool(const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>&)>
        Parallelotope<N, T, Color>::overlap_function() const
{
        return parallelotope_.overlap_function();
}

template <std::size_t N, typename T, typename Color>
Parallelotope<N, T, Color>::Parallelotope(
        const std::type_identity_t<T> metalness,
        const std::type_identity_t<T> roughness,
        const Color& color,
        const std::type_identity_t<T> alpha,
        const numerical::Vector<N, T>& org,
        const std::array<numerical::Vector<N, T>, N>& vectors)
        : parallelotope_(org, vectors),
          roughness_(std::clamp(roughness, T{0}, T{1})),
          colors_(shading::ggx::compute_metalness(color.clamp(0, 1), std::clamp(metalness, T{0}, T{1}))),
          alpha_(std::clamp(alpha, T{0}, T{1}))
{
}

template <std::size_t N, typename T, typename Color>
const geometry::spatial::Parallelotope<N, T>& Parallelotope<N, T, Color>::parallelotope() const
{
        return parallelotope_;
}

template <std::size_t N, typename T, typename Color>
const LightSource<N, T, Color>* Parallelotope<N, T, Color>::light_source() const
{
        return nullptr;
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

template <std::size_t N, typename T, typename Color>
T Parallelotope<N, T, Color>::alpha() const
{
        return alpha_;
}

#define TEMPLATE(N, T, C) template class Parallelotope<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
