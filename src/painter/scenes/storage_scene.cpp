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

#include "storage_scene.h"

#include "../shapes/ray_intersection.h"

#include <src/color/color.h>
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/accelerators/bvh_objects.h>

#include <optional>

namespace ns::painter
{
namespace
{
template <typename P>
std::vector<P*> to_pointers(const std::vector<std::unique_ptr<P>>& objects)
{
        std::vector<P*> result;
        result.reserve(objects.size());
        for (const std::unique_ptr<P>& p : objects)
        {
                result.push_back(p.get());
        }
        return result;
}

template <std::size_t N, typename T, typename Color>
class Impl final : public Scene<N, T, Color>
{
        static constexpr int RAY_OFFSET_IN_EPSILONS = 1000;

        inline static thread_local std::int_fast64_t thread_ray_count_ = 0;

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes_;
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources_;
        std::unique_ptr<const Projector<N, T>> projector_;
        Color background_light_;

        std::vector<const LightSource<N, T, Color>*> light_source_pointers_;

        geometry::Bvh<N, T> bvh_;
        T ray_offset_;

        std::tuple<T, const Surface<N, T, Color>*> intersect(const Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;

                const Ray<N, T> ray_moved = Ray<N, T>(ray).move(ray_offset_);

                const auto intersection = bvh_.intersect(
                        ray_moved, Limits<T>::max(),
                        [shapes = &shapes_, &ray_moved](const auto& indices, const auto& max_distance)
                                -> std::optional<std::tuple<T, const Surface<N, T, Color>*>>
                        {
                                const std::tuple<T, const Surface<N, T, Color>*> info =
                                        ray_intersection(*shapes, indices, ray_moved, max_distance);
                                if (std::get<1>(info) != nullptr)
                                {
                                        return info;
                                }
                                return std::nullopt;
                        });
                if (intersection)
                {
                        return *intersection;
                }
                return {0, nullptr};
        }

        const std::vector<const LightSource<N, T, Color>*>& light_sources() const override
        {
                return light_source_pointers_;
        }

        const Projector<N, T>& projector() const override
        {
                return *projector_;
        }

        const Color& background_light() const override
        {
                return background_light_;
        }

        long long thread_ray_count() const noexcept override
        {
                return thread_ray_count_;
        }

public:
        Impl(const Color& background_light,
             std::unique_ptr<const Projector<N, T>>&& projector,
             std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
             std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
             ProgressRatio* const progress)
                : shapes_(std::move(shapes)),
                  light_sources_(std::move(light_sources)),
                  projector_(std::move(projector)),
                  background_light_(background_light),
                  light_source_pointers_(to_pointers(light_sources_)),
                  bvh_(geometry::bvh_objects(shapes_), progress),
                  ray_offset_(bvh_.bounding_box().diagonal().norm() * (RAY_OFFSET_IN_EPSILONS * Limits<T>::epsilon()))
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Scene<N, T, Color>> create_storage_scene(
        const Color& background_light,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes,
        ProgressRatio* const progress)
{
        return std::make_unique<Impl<N, T, Color>>(
                background_light, std::move(projector), std::move(light_sources), std::move(shapes), progress);
}

#define CREATE_STORAGE_SCENE_INSTANTIATION_N_T_C(N, T, C)                     \
        template std::unique_ptr<Scene<(N), T, C>> create_storage_scene(      \
                const C&, std::unique_ptr<const Projector<(N), T>>&&,         \
                std::vector<std::unique_ptr<const LightSource<(N), T, C>>>&&, \
                std::vector<std::unique_ptr<const Shape<(N), T, C>>>&&, ProgressRatio*);

#define CREATE_STORAGE_SCENE_INSTANTIATION_N_T(N, T)                   \
        CREATE_STORAGE_SCENE_INSTANTIATION_N_T_C((N), T, color::Color) \
        CREATE_STORAGE_SCENE_INSTANTIATION_N_T_C((N), T, color::Spectrum)

#define CREATE_STORAGE_SCENE_INSTANTIATION_N(N)            \
        CREATE_STORAGE_SCENE_INSTANTIATION_N_T((N), float) \
        CREATE_STORAGE_SCENE_INSTANTIATION_N_T((N), double)

CREATE_STORAGE_SCENE_INSTANTIATION_N(3)
CREATE_STORAGE_SCENE_INSTANTIATION_N(4)
CREATE_STORAGE_SCENE_INSTANTIATION_N(5)
CREATE_STORAGE_SCENE_INSTANTIATION_N(6)
}
