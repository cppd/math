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

#include <src/progress/progress.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
class ObjectBvh final
{
        Bvh<N, T> bvh_;

public:
        ObjectBvh(std::vector<BvhObject<N, T>>&& objects, ProgressRatio* const progress)
                : bvh_(std::move(objects), progress)
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

        template <typename RayIntersection>
        auto intersect(const Ray<N, T>& ray, const T& max_distance, const RayIntersection& ray_intersection) const
                -> decltype(ray_intersection(std::vector<unsigned>(), T()))
        {
                using Tuple = decltype(ray_intersection(std::vector<unsigned>(), T()));
                static_assert(2 == std::tuple_size_v<Tuple>);
                static_assert(std::is_same_v<T, std::tuple_element_t<0, Tuple>>);
                static_assert(std::is_pointer_v<std::tuple_element_t<1, Tuple>>);

                using Object = std::remove_pointer_t<std::tuple_element_t<1, Tuple>>;

                struct Info
                {
                        T distance;
                        const Object* object;
                        explicit Info(const std::tuple<T, const Object*>& intersection)
                                : distance(std::get<0>(intersection)), object(std::get<1>(intersection))
                        {
                        }
                };

                const auto f = [&ray_intersection](const auto& object_indices, const T& distance) -> std::optional<Info>
                {
                        Info info(ray_intersection(object_indices, distance));
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
