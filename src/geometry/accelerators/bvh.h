/*
Copyright (C) 2017-2023 Topological Manifold

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
#include "bvh_stack.h"

#include "../spatial/bounding_box.h"

#include <src/com/error.h>
#include <src/progress/progress.h>

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace ns::geometry::accelerators
{
namespace bvh_implementation
{
template <std::size_t N, typename T>
struct Node final
{
        spatial::BoundingBox<N, T> bounds;

        union
        {
                std::uint32_t object_offset;
                std::uint32_t second_child_offset;
        };

        std::uint16_t object_count;
        std::uint8_t axis;
};

template <std::size_t N, typename T, typename ObjectIntersect>
class Intersect final
{
        using Result = std::invoke_result_t<ObjectIntersect, std::span<const unsigned>&&, const T&>;

        static constexpr bool TERMINATE_ON_FIRST_HIT = std::is_same_v<Result, bool>;

        const std::vector<unsigned>* const object_indices_;
        const std::vector<Node<N, T>>* const nodes_;
        const ObjectIntersect* const object_intersect_;
        const Ray<N, T>* const ray_;

        const Vector<N, T> dir_reciprocal_;
        const Vector<N, bool> dir_negative_;

        T distance_;
        unsigned node_index_ = 0;
        Result result_{};
        BvhStack stack_;

        void push(const Node<N, T>& node)
        {
                if (dir_negative_[node.axis])
                {
                        stack_.push(node_index_ + 1);
                        node_index_ = node.second_child_offset;
                }
                else
                {
                        stack_.push(node.second_child_offset);
                        ++node_index_;
                }
        }

        [[nodiscard]] bool pop()
        {
                if (stack_.empty())
                {
                        return false;
                }
                node_index_ = stack_.pop();
                return true;
        }

        [[nodiscard]] bool traverse()
        {
                const Node<N, T>& node = (*nodes_)[node_index_];

                if (!node.bounds.intersect(ray_->org(), dir_reciprocal_, dir_negative_, distance_))
                {
                        return pop();
                }

                if (node.object_count == 0)
                {
                        push(node);
                        return true;
                }

                auto info = (*object_intersect_)(
                        std::span(object_indices_->data() + node.object_offset, node.object_count),
                        std::as_const(distance_));

                static_assert(std::is_same_v<decltype(info), decltype(result_)>);

                if constexpr (TERMINATE_ON_FIRST_HIT)
                {
                        if (info)
                        {
                                result_ = true;
                                return false;
                        }
                }
                else
                {
                        static_assert(std::is_same_v<T, std::remove_reference_t<decltype(std::get<0>(*info))>>);
                        if (info)
                        {
                                ASSERT(std::get<0>(*info) < distance_);
                                distance_ = std::get<0>(*info);
                                result_ = std::move(*info);
                        }
                }

                return pop();
        }

public:
        Intersect(
                const std::vector<unsigned>* const object_indices,
                const std::vector<Node<N, T>>* const nodes,
                const Ray<N, T>* const ray,
                const T& max_distance,
                const ObjectIntersect* const object_intersect)
                : object_indices_(object_indices),
                  nodes_(nodes),
                  object_intersect_(object_intersect),
                  ray_(ray),
                  dir_reciprocal_(ray->dir().reciprocal()),
                  dir_negative_(ray->dir().negative_bool()),
                  distance_(max_distance)
        {
        }

        [[nodiscard]] Result compute()
        {
                while (traverse())
                {
                }
                return result_;
        }
};
}

template <std::size_t N, typename T>
class Bvh final
{
        static_assert(
                N != 3 || !(std::is_same_v<float, T>)
                || sizeof(bvh_implementation::Node<N, T>) == 6 * sizeof(float) + 2 * sizeof(std::uint32_t));

        std::vector<unsigned> object_indices_;
        std::vector<bvh_implementation::Node<N, T>> nodes_;

public:
        explicit Bvh(std::vector<BvhObject<N, T>>&& objects, progress::Ratio* progress);

        [[nodiscard]] const spatial::BoundingBox<N, T>& bounding_box() const
        {
                return nodes_[0].bounds;
        }

        [[nodiscard]] std::optional<T> intersect_root(const Ray<N, T>& ray, const T& max_distance) const
        {
                return nodes_[0].bounds.intersect_volume(ray, max_distance);
        }

        // The signature of the object_intersect function
        // std::optional<std::tuple<T, ...> f(const auto& indices, const auto& max_distance);
        // bool f(const auto& indices, const auto& max_distance);
        template <typename ObjectIntersect>
        [[nodiscard]] std::invoke_result_t<ObjectIntersect, std::span<const unsigned>&&, const T&> intersect(
                const Ray<N, T>& ray,
                const T& max_distance,
                const ObjectIntersect& object_intersect) const
        {
                return bvh_implementation::Intersect<N, T, ObjectIntersect>(
                               &object_indices_, &nodes_, &ray, max_distance, &object_intersect)
                        .compute();
        }
};
}
