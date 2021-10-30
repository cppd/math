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

#include "bvh_functions.h"
#include "bvh_object.h"
#include "bvh_split.h"

#include "testing/bounding_box_intersection.h"

#include <src/com/error.h>

#include <array>
#include <utility>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
struct BvhBuildNode final
{
        BoundingBox<N, T> bounds;
        std::array<unsigned, 2> children;
        unsigned axis;
        unsigned object_index_offset;
        unsigned object_index_count;

        BvhBuildNode()
        {
        }

        // leaf
        BvhBuildNode(
                const BoundingBox<N, T>& bounds,
                const unsigned object_index_offset,
                const unsigned object_index_count)
                : bounds(bounds), object_index_offset(object_index_offset), object_index_count(object_index_count)
        {
                ASSERT(object_index_count > 0);
        }

        // interior
        BvhBuildNode(
                const BoundingBox<N, T>& bounds,
                const unsigned axis,
                const unsigned child_0,
                const unsigned child_1)
                : bounds(bounds), children{child_0, child_1}, axis(axis), object_index_count(0)
        {
        }
};

template <std::size_t N, typename T>
class BvhBuild final
{
        static void clear(std::vector<BvhObject<N, T>>& v)
        {
                v.clear();
                v.shrink_to_fit();
        }

        const T interior_node_traversal_cost_ = spatial::testing::bounding_box::intersection_r_cost<N, T>();
        std::vector<unsigned> object_indices_;
        std::vector<BvhBuildNode<N, T>> nodes_;

        unsigned reserve_indices(const unsigned count)
        {
                const unsigned offset = object_indices_.size();
                object_indices_.resize(offset + count);
                return offset;
        }

        unsigned reserve_node()
        {
                const unsigned offset = nodes_.size();
                nodes_.emplace_back();
                return offset;
        }

        void build(const unsigned node_index, const BoundingBox<N, T>& bounds, std::vector<BvhObject<N, T>>& objects)
        {
                if (std::optional<BvhSplit<N, T>> s =
                            split(std::as_const(objects), bounds, interior_node_traversal_cost_))
                {
                        clear(objects);
                        const unsigned child_0 = reserve_node();
                        build(child_0, s->bounds_min, s->objects_min);
                        const unsigned child_1 = reserve_node();
                        build(child_1, s->bounds_max, s->objects_max);
                        nodes_[node_index] = BvhBuildNode<N, T>(bounds, s->axis, child_0, child_1);
                }
                else
                {
                        unsigned offset = reserve_indices(objects.size());
                        for (const BvhObject<N, T>& object : objects)
                        {
                                object_indices_[offset++] = object.index;
                        }
                        clear(objects);
                        nodes_[node_index] = BvhBuildNode<N, T>(bounds, offset, objects.size());
                }
        }

public:
        BvhBuild(std::vector<BvhObject<N, T>>&& objects)
        {
                const unsigned root = reserve_node();
                ASSERT(root == 0);
                build(root, compute_bounds(objects), objects);
        }

        std::vector<unsigned>& object_indices()
        {
                return object_indices_;
        }

        std::vector<BvhBuildNode<N, T>>& nodes()
        {
                return nodes_;
        }
};
}
