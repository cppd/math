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

/*
R. Stuart Ferguson.
Practical Algorithms For 3D Computer Graphics, Second Edition.
CRC Press, 2014.

5.3.4 Octree decomposition
*/

#pragma once

#include "../spatial/bounding_box.h"

#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <array>
#include <optional>
#include <type_traits>
#include <vector>

namespace ns::geometry::accelerators
{
template <typename Parallelotope>
class SpatialSubdivisionTree final
{
        static constexpr int CHILD_COUNT = 1u << Parallelotope::SPACE_DIMENSION;

        struct Box final
        {
                Parallelotope parallelotope;
                std::vector<int> object_indices;
                std::array<int, CHILD_COUNT> childs;

                explicit Box(Parallelotope&& parallelotope)
                        : parallelotope(std::move(parallelotope))
                {
                }
        };

        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = Parallelotope::DataType;
        static constexpr int ROOT_BOX = 0;

        std::vector<Box> boxes_;
        T ray_offset_;

        [[nodiscard]] const Box* find_box_for_point(const Box& box, const Vector<N, T>& p) const
        {
                if (!box.parallelotope.inside(p))
                {
                        return nullptr;
                }
                if (box.childs[0] < 0)
                {
                        return &box;
                }
                for (const int child_box : box.childs)
                {
                        const Box* const b = find_box_for_point(boxes_[child_box], p);
                        if (b)
                        {
                                return b;
                        }
                }
                return nullptr;
        }

        [[nodiscard]] const Box* find_box_for_point(const Vector<N, T>& p) const
        {
                return find_box_for_point(boxes_[ROOT_BOX], p);
        }

        [[nodiscard]] bool find_next_box(const Ray<N, T>& ray, const Box** const box, Vector<N, T>* const point) const
        {
                T offset = ray_offset_;
                T k = 1;

                while (true)
                {
                        const Vector<N, T> p = ray.point(offset);
                        const Box* const next_box = find_box_for_point(p);

                        if (!next_box)
                        {
                                return false;
                        }

                        if (next_box != *box)
                        {
                                *box = next_box;
                                *point = p;
                                return true;
                        }

                        if (k >= T{1e10})
                        {
                                return false;
                        }

                        k *= 2;
                        offset = k * ray_offset_;
                }
        }

public:
        class Objects
        {
        protected:
                ~Objects() = default;

        public:
                [[nodiscard]] virtual int count() const = 0;

                [[nodiscard]] virtual const spatial::BoundingBox<N, T>& bounding_box() const = 0;

                [[nodiscard]] virtual std::vector<int> intersection_indices(
                        const Parallelotope& parallelotope,
                        const std::vector<int>& object_indices) const = 0;
        };

        SpatialSubdivisionTree(const Objects& objects, progress::Ratio* progress);

        [[nodiscard]] const Parallelotope& root() const
        {
                return boxes_[ROOT_BOX].parallelotope;
        }

        [[nodiscard]] std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return boxes_[ROOT_BOX].parallelotope.intersect_volume(ray);
        }

        // This function is called after intersect_root.
        // root_t is the intersection found by intersect_root.
        //
        // The signature of the object_intersect function
        // std::optional<std::tuple<T, ...> f(const auto& indices);
        template <typename ObjectIntersect>
        [[nodiscard]] std::invoke_result_t<ObjectIntersect, const std::vector<int>&> intersect(
                const Ray<N, T>& ray,
                const T& root_t,
                const ObjectIntersect& object_intersect) const
        {
                Ray<N, T> local_ray = ray;
                Vector<N, T> point = local_ray.point(root_t);
                local_ray.set_org(point);

                const Box* box = find_box_for_point(point);
                if (!box)
                {
                        box = find_box_for_point(local_ray.point(ray_offset_));
                        if (!box)
                        {
                                return std::nullopt;
                        }
                }

                while (true)
                {
                        if (!box->object_indices.empty())
                        {
                                const auto info = object_intersect(box->object_indices);
                                if (info && box->parallelotope.inside(ray.point(std::get<0>(*info))))
                                {
                                        return info;
                                }
                        }

                        const T next = box->parallelotope.intersect_farthest(local_ray).value_or(0);

                        local_ray.set_org(local_ray.point(next));

                        if (!find_next_box(local_ray, &box, &point))
                        {
                                return std::nullopt;
                        }

                        local_ray->set_org(point);
                }
        }
};
}
