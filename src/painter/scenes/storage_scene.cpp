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

#include "ray_intersection.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/geometry/spatial/tree.h>

#include <optional>

namespace ns::painter
{
namespace
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <std::size_t N, typename T>
using TreeParallelotope = geometry::ParallelotopeAA<N, T>;

template <std::size_t N, typename T>
using Tree = geometry::SpatialSubdivisionTree<TreeParallelotope<N, T>>;

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
class Intersections final : public Tree<N, T>::ObjectIntersections
{
        std::vector<std::function<bool(const geometry::ShapeIntersection<TreeParallelotope<N, T>>&)>> wrappers_;

        std::vector<int> indices(const TreeParallelotope<N, T>& parallelotope, const std::vector<int>& indices)
                const override
        {
                geometry::ShapeIntersection p(&parallelotope);
                std::vector<int> intersections;
                intersections.reserve(indices.size());
                for (int object_index : indices)
                {
                        if (wrappers_[object_index](p))
                        {
                                intersections.push_back(object_index);
                        }
                }
                return intersections;
        }

public:
        explicit Intersections(const std::vector<const Shape<N, T, Color>*>& shapes)
        {
                wrappers_.reserve(shapes.size());
                for (const Shape<N, T, Color>* s : shapes)
                {
                        wrappers_.push_back(s->intersection_function());
                }
        }
};

template <std::size_t N, typename T, typename Color>
Tree<N, T> create_tree(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const geometry::BoundingBox<N, T>& bounding_box)
{
        ProgressRatio progress(nullptr);

        const unsigned thread_count = hardware_concurrency();

        Tree<N, T> tree(
                TREE_MIN_OBJECTS_PER_BOX, shapes.size(), bounding_box, Intersections(shapes), thread_count, &progress);

        return tree;
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

        Tree<N, T> tree_;

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;

                const Ray<N, T> ray_with_offset = Ray<N, T>(ray).move(ray_offset_);

                std::optional<T> root = tree_.intersect_root(ray_with_offset);
                if (!root)
                {
                        return nullptr;
                }

                struct Info
                {
                        Vector<N, T> point;
                        const Surface<N, T, Color>* surface;
                        explicit Info(const ShapeIntersection<N, T, Color>& intersection)
                                : surface(intersection.surface)
                        {
                        }
                };

                const auto f = [&](const std::vector<int>& shape_indices) -> std::optional<Info>
                {
                        Info info(ray_intersection(shape_pointers_, shape_indices, ray_with_offset));
                        if (info.surface)
                        {
                                info.point = info.surface->point();
                                return info;
                        }
                        return std::nullopt;
                };

                const auto info = tree_.trace_ray(ray_with_offset, *root, f);
                return info ? info->surface : nullptr;
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
                  tree_(create_tree(shape_pointers_, bounding_box))
        {
                ASSERT(projector_);
        }

public:
        Impl(const Color& background_light,
             std::unique_ptr<const Projector<N, T>>&& projector,
             std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
             std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes)
                : Impl(compute_bounding_box(shapes),
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
