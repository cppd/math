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

#include "scene.h"

#include "ray_intersection.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/accelerators/bvh_objects.h>
#include <src/geometry/spatial/clip_plane.h>
#include <src/geometry/spatial/convex_polytope.h>
#include <src/geometry/spatial/point_offset.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter::scenes
{
namespace
{
template <std::size_t N, typename T>
[[nodiscard]] std::optional<geometry::spatial::ConvexPolytope<N - 1, T>> clip_plane_to_clip_polytope(
        const std::optional<numerical::Vector<N, T>>& clip_plane_equation)
{
        if (!clip_plane_equation)
        {
                return std::nullopt;
        }
        return geometry::spatial::ConvexPolytope<N - 1, T>{
                {geometry::spatial::clip_plane_equation_to_clip_plane(*clip_plane_equation)}};
}

template <std::size_t N, typename T, typename Color, bool USE_CLIP_POLYTOPE>
class Impl final : public Scene<N, T, Color>
{
        inline static thread_local std::int_fast64_t thread_ray_count_ = 0;

        const Color background_color_;
        const std::vector<const Shape<N, T, Color>*> shapes_;
        const std::vector<const LightSource<N, T, Color>*> light_sources_;
        const Projector<N, T>* const projector_;
        const std::optional<geometry::spatial::ConvexPolytope<N, T>> clip_polytope_;
        const geometry::accelerators::Bvh<N, T> bvh_;

        [[nodiscard]] bool move_ray(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                numerical::Ray<N, T>* const ray,
                T* const max_distance) const
        {
                if (geometric_normal)
                {
                        ray->set_org(geometry::spatial::offset_ray_org(*geometric_normal, *ray));
                }

                if constexpr (!USE_CLIP_POLYTOPE)
                {
                        ASSERT(!clip_polytope_);
                        return true;
                }

                ASSERT(clip_polytope_);
                T near = 0;
                if (clip_polytope_->intersect(*ray, &near, max_distance))
                {
                        ray->move(near);
                        *max_distance -= near;
                        return true;
                }
                return false;
        }

        [[nodiscard]] SurfaceIntersection<N, T, Color> intersect_impl(
                const numerical::Ray<N, T>& ray,
                const T max_distance) const
        {
                const auto intersection = bvh_.intersect(
                        ray, max_distance,
                        [shapes = &shapes_, &ray](const auto& indices, const auto& max)
                                -> std::optional<std::tuple<T, const Surface<N, T, Color>*>>
                        {
                                const ShapeIntersection<N, T, Color> info =
                                        ray_intersection(*shapes, indices, ray, max);
                                if (info.surface)
                                {
                                        return {
                                                {info.distance, info.surface}
                                        };
                                }
                                return {};
                        });

                if (intersection)
                {
                        return {std::get<1>(*intersection), ray, std::get<0>(*intersection)};
                }

                return {};
        }

        [[nodiscard]] bool intersect_any_impl(const numerical::Ray<N, T>& ray, const T max_distance) const
        {
                return bvh_.intersect(
                        ray, max_distance,
                        [shapes = &shapes_, &ray](const auto& indices, const auto& max) -> bool
                        {
                                return ray_intersection_any(*shapes, indices, ray, max);
                        });
        }

        [[nodiscard]] SurfaceIntersection<N, T, Color> intersect_impl(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                numerical::Ray<N, T> ray,
                T max_distance) const
        {
                if (move_ray(geometric_normal, &ray, &max_distance))
                {
                        return intersect_impl(ray, max_distance);
                }
                return {};
        }

        [[nodiscard]] bool intersect_any_impl(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                numerical::Ray<N, T> ray,
                T max_distance) const
        {
                if (move_ray(geometric_normal, &ray, &max_distance))
                {
                        return intersect_any_impl(ray, max_distance);
                }
                return false;
        }

        //

        [[nodiscard]] SurfaceIntersection<N, T, Color> intersect(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;
                return intersect_impl(geometric_normal, ray, Limits<T>::infinity());
        }

        [[nodiscard]] SurfaceIntersection<N, T, Color> intersect(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray,
                const T max_distance) const override
        {
                ASSERT(max_distance > 0);

                ++thread_ray_count_;
                return intersect_impl(geometric_normal, ray, max_distance);
        }

        [[nodiscard]] bool intersect_any(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray,
                const T max_distance) const override
        {
                ASSERT(max_distance > 0);

                ++thread_ray_count_;
                return intersect_any_impl(geometric_normal, ray, max_distance);
        }

        [[nodiscard]] const std::vector<const LightSource<N, T, Color>*>& light_sources() const override
        {
                return light_sources_;
        }

        [[nodiscard]] const Color& background_color() const override
        {
                return background_color_;
        }

        [[nodiscard]] const Projector<N, T>& projector() const override
        {
                return *projector_;
        }

        [[nodiscard]] long long thread_ray_count() const noexcept override
        {
                return thread_ray_count_;
        }

public:
        Impl(const Color& background_color,
             const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
             const Projector<N, T>* const projector,
             std::vector<const LightSource<N, T, Color>*>&& light_sources,
             std::vector<const Shape<N, T, Color>*>&& shapes,
             progress::Ratio* const progress)
                : background_color_(background_color),
                  shapes_(std::move(shapes)),
                  light_sources_(std::move(light_sources)),
                  projector_(projector),
                  clip_polytope_(clip_plane_to_clip_polytope(clip_plane_equation)),
                  bvh_(geometry::accelerators::bvh_objects(shapes_), progress)
        {
                ASSERT(USE_CLIP_POLYTOPE == clip_polytope_.has_value());
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_scene(
        const Color& background_color,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        const Projector<N, T>* const projector,
        std::vector<const LightSource<N, T, Color>*>&& light_sources,
        std::vector<const Shape<N, T, Color>*>&& shapes,
        progress::Ratio* const progress)
{
        if (clip_plane_equation)
        {
                return std::make_unique<Impl<N, T, Color, true>>(
                        background_color, clip_plane_equation, projector, std::move(light_sources), std::move(shapes),
                        progress);
        }

        return std::make_unique<Impl<N, T, Color, false>>(
                background_color, clip_plane_equation, projector, std::move(light_sources), std::move(shapes),
                progress);
}

#define TEMPLATE(N, T, C)                                                                                \
        template std::unique_ptr<const Scene<(N), T, C>> create_scene(                                   \
                const C&, const std::optional<numerical::Vector<(N) + 1, T>>&, const Projector<(N), T>*, \
                std::vector<const LightSource<(N), T, C>*>&&, std::vector<const Shape<(N), T, C>*>&&,    \
                progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
