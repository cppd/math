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

/*
R. Stuart Ferguson.
Practical Algorithms For 3D Computer Graphics, Second Edition.
CRC Press, 2014.

5.3.4 Octree decomposition
*/

#pragma once

#include "../spatial/bounding_box.h"

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::geometry
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

                explicit Box(Parallelotope&& parallelotope) : parallelotope(std::move(parallelotope))
                {
                }
        };

        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;
        static constexpr int ROOT_BOX = 0;

        std::vector<Box> boxes_;
        T ray_offset_;

        const Box* find_box_for_point(const Box& box, const Vector<N, T>& p) const
        {
                if (!box.parallelotope.inside(p))
                {
                        return nullptr;
                }
                if (box.childs[0] < 0)
                {
                        return &box;
                }
                for (int child_box : box.childs)
                {
                        const Box* const b = find_box_for_point(boxes_[child_box], p);
                        if (b)
                        {
                                return b;
                        }
                }
                return nullptr;
        }

        const Box* find_box_for_point(const Vector<N, T>& p) const
        {
                return find_box_for_point(boxes_[ROOT_BOX], p);
        }

public:
        class ObjectIntersections
        {
        protected:
                ~ObjectIntersections() = default;

        public:
                virtual std::vector<int> indices(
                        const Parallelotope& parallelotope,
                        const std::vector<int>& object_indices) const = 0;
        };

        SpatialSubdivisionTree(
                int min_objects_per_box,
                int object_count,
                const BoundingBox<N, T>& bounding_box,
                const ObjectIntersections& object_intersections,
                unsigned thread_count,
                ProgressRatio* progress);

        const Parallelotope& root() const
        {
                return boxes_[ROOT_BOX].parallelotope;
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return boxes_[ROOT_BOX].parallelotope.intersect_volume(ray);
        }

        // This function is called after intersect_root.
        // root_t is the intersection found by intersect_root.
        //
        // The signature of the object_intersect function
        // struct Info
        // {
        //     Vector<N, T> point;
        //     ...
        // };
        // std::optional<Info> f(const std::vector<int>& object_indices);
        template <typename ObjectIntersect>
        std::invoke_result_t<ObjectIntersect, const std::vector<int>&> intersect(
                Ray<N, T> ray,
                const T root_t,
                const ObjectIntersect& object_intersect) const
        {
                const Box* box;
                Vector<N, T> point;

                point = ray.point(root_t);
                ray.set_org(point);
                box = find_box_for_point(point);
                if (!box)
                {
                        box = find_box_for_point(ray.point(ray_offset_));
                        if (!box)
                        {
                                return std::nullopt;
                        }
                }

                while (true)
                {
                        if (!box->object_indices.empty())
                        {
                                auto info = object_intersect(box->object_indices);
                                if (info && box->parallelotope.inside(info->point))
                                {
                                        return info;
                                }
                        }

                        std::optional<T> next = box->parallelotope.intersect_farthest(ray);
                        if (!next)
                        {
                                next = 0;
                        }

                        ray.set_org(ray.point(*next));

                        T offset = ray_offset_;
                        T k = 1;
                        while (true)
                        {
                                point = ray.point(offset);
                                const Box* next_box = find_box_for_point(point);
                                if (!next_box)
                                {
                                        return std::nullopt;
                                }
                                if (next_box != box)
                                {
                                        box = next_box;
                                        ray.set_org(point);
                                        break;
                                }

                                if (k >= T(1e10))
                                {
                                        return std::nullopt;
                                }
                                k *= 2;
                                offset = k * ray_offset_;
                        }
                }
        }
};
}
