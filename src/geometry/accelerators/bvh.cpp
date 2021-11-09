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

#include "bvh.h"

#include "bvh_build.h"

namespace ns::geometry
{
namespace
{
template <std::size_t N, typename T, typename Node>
unsigned make_depth_first_order(
        const BvhBuild<N, T>& build,
        const unsigned src_index,
        std::vector<unsigned>* object_indices,
        std::vector<Node>* nodes)
{
        const unsigned dst_index = nodes->size();
        Node& dst = nodes->emplace_back();

        const BvhBuildNode<N, T>& src = build.nodes()[src_index];

        dst.bounds = src.bounds;
        if (src.object_index_count == 0)
        {
                dst.object_count = 0;
                dst.axis = src.axis;
                make_depth_first_order(build, src.children[0], object_indices, nodes);
                dst.second_child_offset = make_depth_first_order(build, src.children[1], object_indices, nodes);
        }
        else
        {
                dst.object_offset = object_indices->size();
                dst.object_count = src.object_index_count;
                const auto begin = build.object_indices().cbegin() + src.object_index_offset;
                const auto end = begin + src.object_index_count;
                for (auto iter = begin; iter != end; ++iter)
                {
                        object_indices->push_back(*iter);
                }
        }
        return dst_index;
}
}

template <std::size_t N, typename T>
Bvh<N, T>::Bvh(std::vector<BvhObject<N, T>>&& objects, ProgressRatio* const progress)
{
        BvhBuild build(std::span(std::data(objects), std::size(objects)), progress);

        ASSERT(!build.object_indices().empty());
        ASSERT(!build.nodes().empty());

        object_indices_.reserve(build.object_indices().size());
        nodes_.reserve(build.nodes().size());

        constexpr unsigned ROOT = 0;
        make_depth_first_order(build, ROOT, &object_indices_, &nodes_);

        ASSERT(object_indices_.size() == build.object_indices().size());
        ASSERT(nodes_.size() == build.nodes().size());
}

#define BVH_TREE_INSTANTIATION_N_T(N, T) template class Bvh<(N), T>;

#define BVH_TREE_INSTANTIATION_N(N)            \
        BVH_TREE_INSTANTIATION_N_T((N), float) \
        BVH_TREE_INSTANTIATION_N_T((N), double)

BVH_TREE_INSTANTIATION_N(3)
BVH_TREE_INSTANTIATION_N(4)
BVH_TREE_INSTANTIATION_N(5)
BVH_TREE_INSTANTIATION_N(6)
}
