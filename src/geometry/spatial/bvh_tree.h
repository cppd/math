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
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

4.3 Bounding volume hierarchies
4.3.4 Compact BVH for traversal
*/

#pragma once

#include "bvh_build.h"

#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
class BvhTree final
{
        struct Node final
        {
                BoundingBox<N, T> bounds;
                union
                {
                        uint32_t object_offset;
                        uint32_t second_child_offset;
                };
                uint16_t object_count;
                uint8_t axis;
        };

        static_assert(
                N != 3 || !std::is_same_v<float, T> || sizeof(float) != sizeof(uint32_t)
                || sizeof(Node) == 8 * sizeof(uint32_t));

        std::vector<unsigned> object_indices_;
        std::vector<Node> nodes_;

        unsigned make_depth_first_order(const BvhBuild<N, T>& build, const unsigned src_index)
        {
                const unsigned dst_index = nodes_.size();
                Node& dst = nodes_.emplace_back();

                const BvhBuildNode<N, T>& src = build.nodes()[src_index];

                dst.bounds = src.bounds;
                if (src.object_index_count == 0)
                {
                        dst.object_count = 0;
                        dst.axis = src.axis;
                        make_depth_first_order(build, src.children[0]);
                        dst.second_child_offset = make_depth_first_order(build, src.children[1]);
                }
                else
                {
                        dst.object_offset = object_indices_.size();
                        dst.object_count = src.object_index_count;
                        const auto begin = build.object_indices().cbegin() + src.object_index_offset;
                        const auto end = begin + src.object_index_count;
                        for (auto i = begin; i != end; ++i)
                        {
                                object_indices_.push_back(*i);
                        }
                }
                return dst_index;
        }

public:
        BvhTree(const std::span<BvhObject<N, T>>& objects)
        {
                BvhBuild build(objects);
                ASSERT(!build.object_indices().empty());
                ASSERT(!build.nodes().empty());

                object_indices_.reserve(build.object_indices().size());
                nodes_.reserve(build.nodes().size());

                make_depth_first_order(build, 0);

                ASSERT(object_indices_.size() == build.object_indices().size());
                ASSERT(nodes_.size() == build.nodes().size());
        }
};
}
