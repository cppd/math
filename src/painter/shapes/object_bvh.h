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
#include <src/geometry/accelerators/bvh.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Objects>
class ObjectBvh final
{
        std::vector<geometry::BvhObject<N, T>> bvh_objects(const Objects& shapes)
        {
                std::vector<geometry::BvhObject<N, T>> res;
                res.reserve(shapes.size());
                for (std::size_t i = 0; i < shapes.size(); ++i)
                {
                        res.emplace_back(to_ref(shapes[i]).bounding_box(), to_ref(shapes[i]).intersection_cost(), i);
                }
                return res;
        }

        const Objects* const objects_;
        geometry::Bvh<N, T> bvh_;

        using Object = std::remove_pointer_t<std::remove_reference_t<decltype(std::get<1>(
                ray_intersection(*objects_, std::vector<unsigned>(), Ray<N, T>(), T())))>>;

public:
        ObjectBvh(const Objects* const shapes, ProgressRatio* const progress)
                : objects_(shapes), bvh_(bvh_objects(*shapes), progress)
        {
        }

        decltype(auto) bounding_box() const
        {
                return bvh_.bounding_box();
        }

        std::tuple<T, const Object*> intersect(const Ray<N, T>& ray, const T& max_distance) const
        {
                struct Info
                {
                        T distance;
                        const Object* object;
                        explicit Info(const std::tuple<T, const Object*>& intersection)
                                : distance(std::get<0>(intersection)), object(std::get<1>(intersection))
                        {
                        }
                };

                const auto f = [&](const auto& object_indices, const T& distance) -> std::optional<Info>
                {
                        Info info(ray_intersection(*objects_, object_indices, ray, distance));
                        if (info.object)
                        {
                                return info;
                        }
                        return std::nullopt;
                };

                const auto info = bvh_.intersect(ray, max_distance, f);
                if (info)
                {
                        return {info->distance, info->object};
                }
                return {0, nullptr};
        }
};
}
