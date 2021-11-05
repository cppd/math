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

#include "../shapes/object_tree.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

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
                ASSERT(p);
                result.push_back(p.get());
        }
        return result;
}

template <std::size_t N, typename T, typename Color>
geometry::BoundingBox<N, T> compute_bounding_box(const std::vector<std::unique_ptr<const Shape<N, T, Color>>>& shapes)
{
        geometry::BoundingBox<N, T> bb = shapes[0]->bounding_box();
        for (std::size_t i = 1; i < shapes.size(); ++i)
        {
                bb.merge(shapes[i]->bounding_box());
        }
        return bb;
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

        std::vector<const Shape<N, T, Color>*> shape_pointers_;
        std::vector<const LightSource<N, T, Color>*> light_source_pointers_;

        T ray_offset_;
        ObjectTree<N, T, Color> tree_;

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;

                return tree_.intersect(Ray<N, T>(ray).move(ray_offset_));
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

        Impl(const geometry::BoundingBox<N, T>& bounding_box,
             ProgressRatio&& progress,
             const Color& background_light,
             std::unique_ptr<const Projector<N, T>>&& projector,
             std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
             std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes)
                : shapes_(std::move(shapes)),
                  light_sources_(std::move(light_sources)),
                  projector_(std::move(projector)),
                  background_light_(background_light),
                  shape_pointers_(to_pointers(shapes_)),
                  light_source_pointers_(to_pointers(light_sources_)),
                  ray_offset_(bounding_box.diagonal().norm() * (RAY_OFFSET_IN_EPSILONS * Limits<T>::epsilon())),
                  tree_(&shape_pointers_, bounding_box, &progress)
        {
                ASSERT(projector_);
        }

public:
        Impl(const Color& background_light,
             std::unique_ptr<const Projector<N, T>>&& projector,
             std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
             std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes)
                : Impl(compute_bounding_box(shapes),
                       ProgressRatio(nullptr),
                       background_light,
                       std::move(projector),
                       std::move(light_sources),
                       std::move(shapes))
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Scene<N, T, Color>> create_storage_scene(
        const Color& background_light,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes)
{
        return std::make_unique<Impl<N, T, Color>>(
                background_light, std::move(projector), std::move(light_sources), std::move(shapes));
}

#define CREATE_STORAGE_SCENE_INSTANTIATION_N_T_C(N, T, C)                     \
        template std::unique_ptr<Scene<(N), T, C>> create_storage_scene(      \
                const C&, std::unique_ptr<const Projector<(N), T>>&&,         \
                std::vector<std::unique_ptr<const LightSource<(N), T, C>>>&&, \
                std::vector<std::unique_ptr<const Shape<(N), T, C>>>&&);

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
