/*
Copyright (C) 2017-2022 Topological Manifold

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
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

4.3 Bounding volume hierarchies
*/

#pragma once

#include "bvh_object.h"

#include "../spatial/bounding_box.h"

#include <src/com/error.h>
#include <src/progress/progress.h>

#include <array>
#include <span>
#include <utility>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
class Bvh final
{
        static constexpr unsigned STACK_SIZE = 64;

        class Stack final
        {
                std::array<unsigned, STACK_SIZE> stack_;
                int next_ = 0;

        public:
                void push(const unsigned v)
                {
                        stack_[next_++] = v;
                }
                unsigned pop()
                {
                        return stack_[--next_];
                }
                bool empty() const
                {
                        return next_ == 0;
                }
        };

        struct Node final
        {
                BoundingBox<N, T> bounds;
                union
                {
                        std::uint32_t object_offset;
                        std::uint32_t second_child_offset;
                };
                std::uint16_t object_count;
                std::uint8_t axis;
        };

        static_assert(
                N != 3 || !std::is_same_v<float, T> || sizeof(Node) == 6 * sizeof(float) + 2 * sizeof(std::uint32_t));

        std::vector<unsigned> object_indices_;
        std::vector<Node> nodes_;

public:
        explicit Bvh(std::vector<BvhObject<N, T>>&& objects, ProgressRatio* progress);

        const BoundingBox<N, T>& bounding_box() const
        {
                return nodes_[0].bounds;
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray, const T& max_distance) const
        {
                return nodes_[0].bounds.intersect(ray, max_distance);
        }

        // The signature of the object_intersect function
        // std::optional<std::tuple<T, ...> f(const auto& indices, const auto& max_distance);
        template <typename ObjectIntersect>
        std::invoke_result_t<ObjectIntersect, std::span<const unsigned>&&, const T&> intersect(
                const Ray<N, T>& ray,
                const T& max_distance,
                const ObjectIntersect& object_intersect) const
        {
                const Vector<N, T> dir_reciprocal = reciprocal(ray.dir());
                const Vector<N, bool> dir_negative = negative_bool(ray.dir());

                std::invoke_result_t<ObjectIntersect, std::span<const unsigned>&&, const T&> result;

                Stack stack;
                T distance = max_distance;
                unsigned node_index = 0;

                while (true)
                {
                        const Node& node = nodes_[node_index];
                        if (node.bounds.intersect(ray.org(), dir_reciprocal, dir_negative, distance))
                        {
                                if (node.object_count == 0)
                                {
                                        if (dir_negative[node.axis])
                                        {
                                                stack.push(node_index + 1);
                                                node_index = node.second_child_offset;
                                        }
                                        else
                                        {
                                                stack.push(node.second_child_offset);
                                                ++node_index;
                                        }
                                        continue;
                                }
                                auto info = object_intersect(
                                        std::span(object_indices_.data() + node.object_offset, node.object_count),
                                        std::as_const(distance));
                                static_assert(std::is_same_v<decltype(info), decltype(result)>);
                                static_assert(std::is_same_v<T, std::remove_reference_t<decltype(std::get<0>(*info))>>);
                                if (info)
                                {
                                        ASSERT(std::get<0>(*info) < distance);
                                        distance = std::get<0>(*info);
                                        result = std::move(*info);
                                }
                        }
                        if (stack.empty())
                        {
                                break;
                        }
                        node_index = stack.pop();
                }
                return result;
        }
};
}
