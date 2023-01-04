/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "hyperplane_parallelotope.h"

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
        const HyperplaneParallelotope<N, T, Color>* obj_;

        [[nodiscard]] Vector<N, T> point(const Ray<N, T>& ray, const T distance) const override
        {
                return obj_->hyperplane_parallelotope().project(ray.point(distance));
        }

        [[nodiscard]] Vector<N, T> geometric_normal(const Vector<N, T>& /*point*/) const override
        {
                return obj_->hyperplane_parallelotope().normal();
        }

        [[nodiscard]] std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& /*point*/) const override
        {
                return std::nullopt;
        }

        [[nodiscard]] const LightSource<N, T, Color>* light_source() const override
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
        explicit SurfaceImpl(const HyperplaneParallelotope<N, T, Color>* const obj)
                : obj_(obj)
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
T HyperplaneParallelotope<N, T, Color>::intersection_cost() const
{
        return decltype(hyperplane_parallelotope_)::intersection_cost();
}

template <std::size_t N, typename T, typename Color>
std::optional<T> HyperplaneParallelotope<N, T, Color>::intersect_bounds(const Ray<N, T>& ray, const T max_distance)
        const
{
        if (alpha_nonzero_)
        {
                std::optional<T> res = hyperplane_parallelotope_.intersect(ray);
                if (res && *res < max_distance)
                {
                        return res;
                }
        }
        return std::nullopt;
}

template <std::size_t N, typename T, typename Color>
ShapeIntersection<N, T, Color> HyperplaneParallelotope<N, T, Color>::intersect(
        const Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T bounding_distance) const
{
        return {bounding_distance, make_arena_ptr<SurfaceImpl<N, T, Color>>(this)};
}

template <std::size_t N, typename T, typename Color>
bool HyperplaneParallelotope<N, T, Color>::intersect_any(
        const Ray<N, T>& /*ray*/,
        const T /*max_distance*/,
        const T /*bounding_distance*/) const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
geometry::spatial::BoundingBox<N, T> HyperplaneParallelotope<N, T, Color>::bounding_box() const
{
        return geometry::spatial::BoundingBox<N, T>(hyperplane_parallelotope_.vertices());
}

template <std::size_t N, typename T, typename Color>
std::function<bool(const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>&)>
        HyperplaneParallelotope<N, T, Color>::overlap_function() const
{
        return hyperplane_parallelotope_.overlap_function();
}

template <std::size_t N, typename T, typename Color>
HyperplaneParallelotope<N, T, Color>::HyperplaneParallelotope(
        const std::type_identity_t<T> metalness,
        const std::type_identity_t<T> roughness,
        const Color& color,
        const std::type_identity_t<T> alpha,
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors)
        : hyperplane_parallelotope_(org, vectors),
          roughness_(std::clamp(roughness, T{0}, T{1})),
          colors_(shading::ggx::compute_metalness(color.clamp(0, 1), std::clamp(metalness, T{0}, T{1}))),
          alpha_(std::clamp(alpha, T{0}, T{1}))
{
}

template <std::size_t N, typename T, typename Color>
const geometry::spatial::HyperplaneParallelotope<N, T>& HyperplaneParallelotope<N, T, Color>::hyperplane_parallelotope()
        const
{
        return hyperplane_parallelotope_;
}

template <std::size_t N, typename T, typename Color>
void HyperplaneParallelotope<N, T, Color>::set_light_source(const LightSource<N, T, Color>* const light_source)
{
        light_source_ = light_source;
}

template <std::size_t N, typename T, typename Color>
const LightSource<N, T, Color>* HyperplaneParallelotope<N, T, Color>::light_source() const
{
        return light_source_;
}

template <std::size_t N, typename T, typename Color>
T HyperplaneParallelotope<N, T, Color>::roughness() const
{
        return roughness_;
}

template <std::size_t N, typename T, typename Color>
const shading::Colors<Color>& HyperplaneParallelotope<N, T, Color>::colors() const
{
        return colors_;
}

#define TEMPLATE(N, T, C) template class HyperplaneParallelotope<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
