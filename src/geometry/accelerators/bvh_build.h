/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/thread.h>
#include <src/com/thread_tasks.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace ns::geometry::accelerators
{
template <std::size_t N, typename T>
struct BvhBuildNode final
{
        spatial::BoundingBox<N, T> bounds;
        std::array<unsigned, 2> children;
        unsigned axis;
        unsigned object_index_offset;
        unsigned object_index_count;

        BvhBuildNode()
        {
        }

        // leaf
        BvhBuildNode(
                const spatial::BoundingBox<N, T>& bounds,
                const unsigned object_index_offset,
                const unsigned object_index_count)
                : bounds(bounds),
                  object_index_offset(object_index_offset),
                  object_index_count(object_index_count)
        {
                ASSERT(object_index_count > 0);
        }

        // interior
        BvhBuildNode(
                const spatial::BoundingBox<N, T>& bounds,
                const unsigned axis,
                const unsigned child_0,
                const unsigned child_1)
                : bounds(bounds),
                  children{child_0, child_1},
                  axis(axis),
                  object_index_count(0)
        {
        }
};

template <std::size_t N, typename T>
class BvhBuild final
{
        static std::size_t max_interior_node_count(const std::size_t object_count)
        {
                // Maximum when each leaf node contains only one object.
                // In this case, interior nodes are a full binary tree
                // with 2 * object_count - 1 nodes.
                return 2 * object_count - 1;
        }

        struct Task final
        {
                std::span<BvhObject<N, T>> objects;
                spatial::BoundingBox<N, T> bounds;
                BvhBuildNode<N, T>* node;

                Task(const std::span<BvhObject<N, T>>& objects,
                     const spatial::BoundingBox<N, T>& bounds,
                     BvhBuildNode<N, T>* const node)
                        : objects(objects),
                          bounds(bounds),
                          node(node)
                {
                }
        };

        const T interior_node_traversal_cost_ = 2 * spatial::BoundingBox<N, T>::intersection_r_cost();
        const double max_interior_node_count_reciprocal_;

        std::vector<unsigned> object_indices_;
        unsigned object_indices_size_ = 0;
        std::mutex object_indices_lock_;

        // std::deque to keep object addreses unchanged when inserting
        std::deque<BvhBuildNode<N, T>> nodes_;
        std::mutex nodes_lock_;

        unsigned create_indices(const unsigned count)
        {
                const std::lock_guard lg(object_indices_lock_);
                const unsigned offset = object_indices_size_;
                object_indices_size_ += count;
                return offset;
        }

        struct ChildNode final
        {
                BvhBuildNode<N, T>* node;
                unsigned index;
        };

        std::array<ChildNode, 2> create_child_nodes()
        {
                std::array<ChildNode, 2> res;
                const std::lock_guard lg(nodes_lock_);
                const unsigned offset = nodes_.size();
                res[0] = {.node = &nodes_.emplace_back(), .index = offset};
                res[1] = {.node = &nodes_.emplace_back(), .index = offset + 1};
                return res;
        }

        void build(ThreadTaskManager<Task>* const task_manager, progress::Ratio* const progress)
        {
                while (const auto task = task_manager->get())
                {
                        if (std::optional<BvhSplit<N, T>> s =
                                    split(task->objects, task->bounds, interior_node_traversal_cost_))
                        {
                                const auto [min, max] = create_child_nodes();
                                if ((min.index & 0xfffe) == 0xfffe)
                                {
                                        progress->set(min.index * max_interior_node_count_reciprocal_);
                                }
                                *task->node = BvhBuildNode<N, T>(task->bounds, s->axis, min.index, max.index);
                                task_manager->emplace(s->objects_min, s->bounds_min, min.node);
                                task_manager->emplace(s->objects_max, s->bounds_max, max.node);
                        }
                        else
                        {
                                const unsigned count = task->objects.size();
                                const unsigned offset = create_indices(count);
                                *task->node = BvhBuildNode<N, T>(task->bounds, offset, count);
                                for (auto iter = object_indices_.begin() + offset;
                                     const BvhObject<N, T>& object : task->objects)
                                {
                                        *iter++ = object.index();
                                }
                        }
                }
        }

public:
        explicit BvhBuild(const std::span<BvhObject<N, T>>& objects, progress::Ratio* const progress)
                : max_interior_node_count_reciprocal_(1.0 / max_interior_node_count(objects.size()))
        {
                if (objects.empty())
                {
                        error("No objects to build BVH");
                }

                object_indices_.resize(objects.size());

                nodes_.emplace_back();

                ThreadTasks<Task> tasks;
                tasks.emplace(objects, compute_bounds(objects), &nodes_.front());

                const auto f = [&]
                {
                        try
                        {
                                ThreadTaskManager task_manager(&tasks);
                                build(&task_manager, progress);
                        }
                        catch (...)
                        {
                                tasks.stop();
                                throw;
                        }
                };

                const unsigned thread_count = hardware_concurrency();

                Threads threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add(f);
                }
                threads.join();

                ASSERT(object_indices_size_ == objects.size());
        }

        [[nodiscard]] const std::vector<unsigned>& object_indices() const
        {
                return object_indices_;
        }

        [[nodiscard]] const std::deque<BvhBuildNode<N, T>>& nodes() const
        {
                return nodes_;
        }
};
}
