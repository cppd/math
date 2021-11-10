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

#include <src/com/reference.h>
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>

#include <optional>
#include <span>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Shapes>
class ObjectBvh final
{
        std::vector<geometry::BvhObject<N, T>> bvh_objects(const Shapes& shapes)
        {
                std::vector<geometry::BvhObject<N, T>> res;
                res.reserve(shapes.size());
                for (std::size_t i = 0; i < shapes.size(); ++i)
                {
                        res.emplace_back(to_ref(shapes[i]).bounding_box(), to_ref(shapes[i]).intersection_cost(), i);
                }
                return res;
        }

        const Shapes* const shapes_;
        geometry::Bvh<N, T> bvh_;

public:
        ObjectBvh(const Shapes* const shapes, ProgressRatio* const progress)
                : shapes_(shapes), bvh_(bvh_objects(*shapes), progress)
        {
        }

        decltype(auto) bounding_box() const
        {
                return bvh_.bounding_box();
        }

        const auto* intersect(const Ray<N, T>& ray) const
        {
                using Surface =
                        std::remove_pointer_t<decltype(to_ref(shapes_->front()).intersect(ray, T(), T()).surface)>;

                struct Info
                {
                        T distance;
                        const Surface* surface;
                        explicit Info(const T& distance, const Surface* surface) : distance(distance), surface(surface)
                        {
                        }
                };

                const auto f = [&](const std::span<const unsigned>& object_indices,
                                   const T& distance) -> std::optional<Info>
                {
                        auto intersection = ray_intersection(*shapes_, object_indices, ray, distance);
                        if (intersection.surface)
                        {
                                return Info(intersection.distance, intersection.surface);
                        }
                        return std::nullopt;
                };

                const auto info = bvh_.intersect(ray, Limits<T>::max(), f);
                return info ? info->surface : nullptr;
        }
};
}
