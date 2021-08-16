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

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/geometry/spatial/shape_wrapper.h>
#include <src/geometry/spatial/tree.h>

#include <algorithm>
#include <cmath>

namespace ns::painter
{
namespace
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <std::size_t N>
int tree_max_depth()
{
        static_assert(N >= 3);

        switch (N)
        {
        case 3:
                return 10;
        case 4:
                return 8;
        case 5:
                return 6;
        case 6:
                return 5;
        default:
                // Сумма геометрической прогрессии s = (pow(r, n) - 1) / (r - 1).
                // Для s и r найти n = log(s * (r - 1) + 1) / log(r).
                double s = 1e9;
                double r = std::pow(2, N);
                double n = std::log(s * (r - 1) + 1) / std::log(r);
                return std::max(2.0, std::floor(n));
        }
}

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
geometry::BoundingBox<N, T> compute_bounding_box(const std::vector<const Shape<N, T, Color>*>& shapes)
{
        geometry::BoundingBox<N, T> bb = shapes[0]->bounding_box();
        for (std::size_t i = 1; i < shapes.size(); ++i)
        {
                geometry::BoundingBox<N, T> shape_bb = shapes[i]->bounding_box();
                bb.min = min(bb.min, shape_bb.min);
                bb.max = max(bb.max, shape_bb.max);
        }
        return bb;
}

template <std::size_t N, typename T, typename Color>
struct BoundingIntersection
{
        T distance;
        const Shape<N, T, Color>* shape;

        BoundingIntersection(T distance, const Shape<N, T, Color>* shape) : distance(distance), shape(shape)
        {
        }

        bool operator<(const BoundingIntersection& b) const
        {
                return distance < b.distance;
        }
};

template <std::size_t N, typename T, typename Color>
const Surface<N, T, Color>* ray_intersect(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const std::vector<int>& indices,
        const Ray<N, T>& ray)
{
        if (indices.size() == 1)
        {
                std::optional<T> distance = shapes[indices.front()]->intersect_bounding(ray);
                if (distance)
                {
                        return shapes[indices.front()]->intersect(ray, *distance);
                }
                return nullptr;
        }

        // Объекты могут быть сложными, поэтому перед поиском точного пересечения
        // их надо разместить по возрастанию примерного пересечения.

        thread_local std::vector<BoundingIntersection<N, T, Color>> intersections;
        intersections.clear();
        intersections.reserve(indices.size());
        for (int index : indices)
        {
                std::optional<T> distance = shapes[index]->intersect_bounding(ray);
                if (distance)
                {
                        intersections.emplace_back(*distance, shapes[index]);
                }
        }
        if (intersections.empty())
        {
                return nullptr;
        }

        std::make_heap(intersections.begin(), intersections.end());

        T min_distance_squared = limits<T>::max();
        const Surface<N, T, Color>* closest_surface = nullptr;

        do
        {
                const BoundingIntersection<N, T, Color>& bounding = intersections.front();

                if (min_distance_squared < square(bounding.distance))
                {
                        break;
                }

                const Surface<N, T, Color>* surface = bounding.shape->intersect(ray, bounding.distance);
                if (surface)
                {
                        T distance_squared = (surface->point() - ray.org()).norm_squared();
                        if (distance_squared < min_distance_squared)
                        {
                                min_distance_squared = distance_squared;
                                closest_surface = surface;
                        }
                }

                std::pop_heap(intersections.begin(), intersections.end());
                intersections.pop_back();
        } while (!intersections.empty());

        return closest_surface;
}

template <std::size_t N, typename T, typename Color>
void create_tree(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const geometry::BoundingBox<N, T>& bounding_box,
        geometry::SpatialSubdivisionTree<geometry::ParallelotopeAA<N, T>>* tree,
        ProgressRatio* progress)
{
        std::vector<std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>>
                wrappers;
        wrappers.reserve(shapes.size());
        for (const Shape<N, T, Color>* s : shapes)
        {
                wrappers.push_back(s->intersection_function());
        }

        const auto shape_intersections =
                [&w = std::as_const(wrappers)](
                        const geometry::ParallelotopeAA<N, T>& parallelotope, const std::vector<int>& indices)
        {
                geometry::ShapeWrapperForIntersection p(parallelotope);
                std::vector<int> intersections;
                intersections.reserve(indices.size());
                for (int object_index : indices)
                {
                        if (w[object_index](p))
                        {
                                intersections.push_back(object_index);
                        }
                }
                return intersections;
        };

        const unsigned thread_count = hardware_concurrency();

        tree->decompose(
                tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, wrappers.size(), bounding_box, shape_intersections,
                thread_count, progress);
}

template <std::size_t N, typename T, typename Color>
class SceneImpl final : public Scene<N, T, Color>
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

        geometry::SpatialSubdivisionTree<geometry::ParallelotopeAA<N, T>> tree_;

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const override
        {
                ++thread_ray_count_;

                const Ray<N, T> ray_with_offset = Ray<N, T>(ray).move(ray_offset_);

                std::optional<T> root = tree_.intersect_root(ray_with_offset);
                if (!root)
                {
                        return nullptr;
                }

                const Surface<N, T, Color>* surface;

                const auto f = [&](const std::vector<int>& shape_indices) -> std::optional<Vector<N, T>>
                {
                        surface = ray_intersect(shape_pointers_, shape_indices, ray_with_offset);
                        if (surface)
                        {
                                return surface->point();
                        }
                        return std::nullopt;
                };

                if (tree_.trace_ray(ray_with_offset, *root, f))
                {
                        return surface;
                }

                return nullptr;
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
        SceneImpl(
                const Color& background_light,
                std::unique_ptr<const Projector<N, T>>&& projector,
                std::vector<std::unique_ptr<const LightSource<N, T, Color>>>&& light_sources,
                std::vector<std::unique_ptr<const Shape<N, T, Color>>>&& shapes)
                : shapes_(std::move(shapes)),
                  light_sources_(std::move(light_sources)),
                  projector_(std::move(projector)),
                  background_light_(background_light),
                  shape_pointers_(to_pointers(shapes_)),
                  light_source_pointers_(to_pointers(light_sources_))
        {
                ASSERT(projector_);

                const geometry::BoundingBox<N, T> bounding_box = compute_bounding_box(shape_pointers_);

                const T scene_size = (bounding_box.max - bounding_box.min).norm();
                ray_offset_ = scene_size * (RAY_OFFSET_IN_EPSILONS * limits<T>::epsilon());

                ProgressRatio progress(nullptr);
                create_tree(shape_pointers_, bounding_box, &tree_, &progress);
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
        return std::make_unique<SceneImpl<N, T, Color>>(
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
