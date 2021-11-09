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

#pragma once

#include "ray_intersection.h"

#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class ObjectBvh final
{
        std::vector<geometry::BvhObject<N, T>> bvh_objects(const std::vector<const Shape<N, T, Color>*>& objects)
        {
                std::vector<geometry::BvhObject<N, T>> res;
                res.reserve(objects.size());
                for (std::size_t i = 0; i < objects.size(); ++i)
                {
                        res.emplace_back(objects[i]->bounding_box(), objects[i]->intersection_cost(), i);
                }
                return res;
        }

        const std::vector<const Shape<N, T, Color>*>* const objects_;
        geometry::Bvh<N, T> bvh_;

public:
        ObjectBvh(const std::vector<const Shape<N, T, Color>*>* const objects, ProgressRatio* const progress)
                : objects_(objects), bvh_(bvh_objects(*objects), progress)
        {
        }

        decltype(auto) bounding_box() const
        {
                return bvh_.bounding_box();
        }

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const
        {
                struct Info
                {
                        T distance;
                        const Surface<N, T, Color>* surface;
                        explicit Info(const ShapeIntersection<N, T, Color>& intersection)
                                : distance(intersection.distance), surface(intersection.surface)
                        {
                        }
                };

                const auto f = [&](const std::span<const unsigned>& object_indices,
                                   const T& distance) -> std::optional<Info>
                {
                        ShapeIntersection<N, T, Color> intersection =
                                ray_intersection(*objects_, object_indices, ray, distance);
                        if (intersection.surface)
                        {
                                return Info(intersection);
                        }
                        return std::nullopt;
                };

                const auto info = bvh_.intersect(ray, Limits<T>::max(), f);
                return info ? info->surface : nullptr;
        }
};
}
