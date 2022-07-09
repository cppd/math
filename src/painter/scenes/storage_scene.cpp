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

#include "storage_scene.h"

#include "../shapes/ray_intersection.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/accelerators/bvh_objects.h>
#include <src/geometry/spatial/clip_plane.h>
#include <src/geometry/spatial/convex_polytope.h>
#include <src/geometry/spatial/point_offset.h>
#include <src/settings/instantiation.h>

#include <optional>

namespace ns::painter
{
namespace
{
template <typename P>
[[nodiscard]] std::vector<P*> to_pointers(const std::vector<std::unique_ptr<P>>& objects)
{
        std::vector<P*> result;
        result.reserve(objects.size());
        for (const std::unique_ptr<P>& p : objects)
        {
                result.push_back(p.get());
        }
        return result;
}

template <std::size_t N, typename T>
[[nodiscard]] std::optional<geometry::ConvexPolytope<N - 1, T>> clip_plane_to_clip_polytope(
        const std::optional<Vector<N, T>>& clip_plane_equation)
{
        if (!clip_plane_equation)
        {
                return std::nullopt;
        }
        return geometry::ConvexPolytope<N - 1, T>{{geometry::clip_plane_equation_to_clip_plane(*clip_plane_equation)}};
}

template <std::size_t N, typename T, typename Color, bool USE_CLIP_POLYTOPE>
class Impl final : public Scene<N, T, Color>
{
        inline static thread_local std::int_fast64_t thread_ray_count_ = 0;

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes_;
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources_;
        std::unique_ptr<const Projector<N, T>> projector_;
        Color background_light_;
        std::optional<geometry::ConvexPolytope<N, T>> clip_polytope_;

        std::vector<const LightSource<N, T, Color>*> light_source_pointers_;

        geometry::Bvh<N, T> bvh_;

        [[nodiscard]] bool move_ray(
                const std::optional<Vector<N, T>>& geometric_normal,
                Ray<N, T>* const ray,
                T* const max_distance) const
        {
                if (geometric_normal)
                {
                        ray->set_org(geometry::offset_ray_org(*geometric_normal, *ray));
                }

                if constexpr (!USE_CLIP_POLYTOPE)
                {
                        return true;
                }

                T near = 0;
                if (clip_polytope_->intersect(*ray, &near, max_distance))
                {
                        ray->move(near);
                        *max_distance -= near;
                        return true;
                }
                return false;
        }

        [[nodiscard]] SurfacePoint<N, T, Color> intersect_impl(const Ray<N, T>& ray, const T max_distance) const
        {
                const auto intersection = bvh_.intersect(
                        ray, max_distance,
                        [shapes = &shapes_, &ray](const auto& indices, const auto& max)
                                -> std::optional<std::tuple<T, const Surface<N, T, Color>*>>
                        {
                                const std::tuple<T, const Surface<N, T, Color>*> info =
                                        ray_intersection(*shapes, indices, ray, max);
                                if (std::get<1>(info))
                                {
                                        return info;
                                }
                                return {};
                        });

                if (intersection)
                {
                        return {std::get<1>(*intersection), ray, std::get<0>(*intersection)};
                }

                return {};
        }

        [[nodiscard]] SurfacePoint<N, T, Color> intersect_impl(
                const std::optional<Vector<N, T>>& geometric_normal,
                Ray<N, T> ray,
                T max_distance) const
        {
                if (move_ray(geometric_normal, &ray, &max_distance))
                {
                        return intersect_impl(ray, max_distance);
                }
                return {};
        }

        //

        [[nodiscard]] SurfacePoint<N, T, Color> intersect(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;
                return intersect_impl(geometric_normal, ray, Limits<T>::max());
        }

        [[nodiscard]] SurfacePoint<N, T, Color> intersect(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray,
                const T max_distance) const override
        {
                ASSERT(max_distance > 0);

                ++thread_ray_count_;
                return intersect_impl(geometric_normal, ray, max_distance);
        }

        [[nodiscard]] const std::vector<const LightSource<N, T, Color>*>& light_sources() const override
        {
                return light_source_pointers_;
        }

        [[nodiscard]] const Projector<N, T>& projector() const override
        {
                return *projector_;
        }

        [[nodiscard]] const Color& background_light() const override
        {
                return background_light_;
        }

        [[nodiscard]] long long thread_ray_count() const noexcept override
        {
                return thread_ray_count_;
        }

public:
        Impl(const Color& background_light,
             const std::optional<Vector<N + 1, T>>& clip_plane_equation,
             std::unique_ptr<const Projector<N, T>>&& projector,
             std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
             std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
             progress::Ratio* const progress)
                : shapes_(std::move(shapes)),
                  light_sources_(std::move(light_sources)),
                  projector_(std::move(projector)),
                  background_light_(background_light),
                  clip_polytope_(clip_plane_to_clip_polytope(clip_plane_equation)),
                  light_source_pointers_(to_pointers(light_sources_)),
                  bvh_(geometry::bvh_objects(shapes_), progress)
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Scene<N, T, Color>> create_storage_scene(
        const Color& background_light,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
        progress::Ratio* const progress)
{
        if (clip_plane_equation)
        {
                return std::make_unique<Impl<N, T, Color, true>>(
                        background_light, clip_plane_equation, std::move(projector), std::move(light_sources),
                        std::move(shapes), progress);
        }
        return std::make_unique<Impl<N, T, Color, false>>(
                background_light, clip_plane_equation, std::move(projector), std::move(light_sources),
                std::move(shapes), progress);
}

#define TEMPLATE(N, T, C)                                                                                       \
        template std::unique_ptr<Scene<(N), T, C>> create_storage_scene(                                        \
                const C&, const std::optional<Vector<(N) + 1, T>>&, std::unique_ptr<const Projector<(N), T>>&&, \
                std::vector<std::unique_ptr<const LightSource<(N), T, C>>>&&,                                   \
                std::vector<std::unique_ptr<const Shape<(N), T, C>>>&&, progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
