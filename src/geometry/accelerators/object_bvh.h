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

#include "bvh.h"
#include "ray_intersection.h"

#include <src/progress/progress.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T, typename Object>
class ObjectBvh final
{
        std::vector<BvhObject<N, T>> bvh_objects(const std::vector<Object>& objects)
        {
                const T intersection_cost = Object::intersection_cost();
                std::vector<BvhObject<N, T>> res;
                res.reserve(objects.size());
                for (std::size_t i = 0; i < objects.size(); ++i)
                {
                        res.emplace_back(objects[i].bounding_box(), intersection_cost, i);
                }
                return res;
        }

        const std::vector<Object>* const objects_;
        Bvh<N, T> bvh_;

public:
        ObjectBvh(const std::vector<Object>* const objects, ProgressRatio* progress)
                : objects_(objects), bvh_(bvh_objects(*objects), progress)
        {
        }

        decltype(auto) bounding_box() const
        {
                return bvh_.bounding_box();
        }

        decltype(auto) intersect_root(const Ray<N, T>& ray, const T& max_distance) const
        {
                return bvh_.intersect_root(ray, max_distance);
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

                const auto f = [&](const std::span<const unsigned>& object_indices,
                                   const T& distance) -> std::optional<Info>
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
