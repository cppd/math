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
#include <src/com/thread.h>

#include <array>
#include <future>
#include <mutex>
#include <stack>
#include <thread>
#include <utility>
#include <vector>

namespace ns::geometry
{
namespace bvh_build_implementation
{
template <std::size_t N, typename T>
struct Task final
{
        std::span<BvhObject<N, T>> objects;
        BoundingBox<N, T> bounds;
        unsigned node_index;

        Task()
        {
        }

        Task(const std::span<BvhObject<N, T>>& objects, const BoundingBox<N, T>& bounds, const unsigned node_index)
                : objects(objects), bounds(bounds), node_index(node_index)
        {
        }
};

template <std::size_t N, typename T>
class Tasks final
{
        std::stack<Task<N, T>, std::vector<Task<N, T>>> tasks_;
        std::mutex lock_;

        // If there are no tasks and all thread do nothing, then there will be no more tasks.
        // If there are no tasks and a thread do something, then new tasks can be created.
        // Instead of counting tasks for each thread, the sum of tasks across all threads is used.
        // A thread requires a new tasks without having a task - the sum is the same.
        // A thread requires a new tasks having a task - the sum decreases by 1.
        // A thread gets a new tasks - the sum increases by 1.
        int task_count_ = 0;

public:
        template <typename... Ts>
        void push(Ts&&... vs)
        {
                std::lock_guard lg(lock_);
                tasks_.emplace(std::forward<Ts>(vs)...);
        }

        enum Action
        {
                EXIT,
                PERFORM,
                WAIT
        };

        Action pop(Task<N, T>* const task)
        {
                std::lock_guard lg(lock_);
                if (!task->objects.empty())
                {
                        --task_count_;
                }
                if (!tasks_.empty())
                {
                        *task = std::move(tasks_.top());
                        tasks_.pop();
                        ++task_count_;
                        return PERFORM;
                }
                if (task_count_ > 0)
                {
                        return WAIT;
                }
                return EXIT;
        }
};
}

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
        using Task = bvh_build_implementation::Task<N, T>;
        using Tasks = bvh_build_implementation::Tasks<N, T>;

        const T interior_node_traversal_cost_ = 2 * spatial::testing::bounding_box::intersection_r_cost<N, T>();

        std::vector<unsigned> object_indices_;
        std::mutex object_indices_lock_;

        std::vector<BvhBuildNode<N, T>> nodes_;
        std::mutex nodes_lock_;

        Tasks tasks_;

        unsigned create_indices(const unsigned count)
        {
                std::lock_guard lg(object_indices_lock_);
                const unsigned offset = object_indices_.size();
                object_indices_.resize(offset + count);
                return offset;
        }

        std::array<unsigned, 2> create_nodes()
        {
                std::lock_guard lg(nodes_lock_);
                const unsigned offset = nodes_.size();
                nodes_.emplace_back();
                nodes_.emplace_back();
                return {offset, offset + 1};
        }

        void perform(const Task& task)
        {
                if (std::optional<BvhSplit<N, T>> s = split(task.objects, task.bounds, interior_node_traversal_cost_))
                {
                        const std::array<unsigned, 2> childs = create_nodes();
                        nodes_[task.node_index] = BvhBuildNode<N, T>(task.bounds, s->axis, childs[0], childs[1]);
                        tasks_.push(s->objects_min, s->bounds_min, childs[0]);
                        tasks_.push(s->objects_max, s->bounds_max, childs[1]);
                }
                else
                {
                        unsigned offset = create_indices(task.objects.size());
                        for (const BvhObject<N, T>& object : task.objects)
                        {
                                object_indices_[offset++] = object.index;
                        }
                        nodes_[task.node_index] = BvhBuildNode<N, T>(task.bounds, offset, task.objects.size());
                }
        }

        void build()
        {
                Task task;
                while (true)
                {
                        switch (tasks_.pop(&task))
                        {
                        case Tasks::EXIT:
                                return;
                        case Tasks::WAIT:
                                break;
                        case Tasks::PERFORM:
                                perform(task);
                                break;
                        }
                }
        }

        void run()
        {
                const auto f = [this]
                {
                        build();
                };

                const int thread_count = hardware_concurrency();

                std::vector<std::future<void>> futures;
                std::vector<std::thread> threads;
                for (int i = 0; i < thread_count; ++i)
                {
                        std::packaged_task<void()> task(f);
                        futures.emplace_back(task.get_future());
                        threads.emplace_back(std::move(task));
                }
                for (std::thread& thread : threads)
                {
                        thread.join();
                }
                for (std::future<void>& future : futures)
                {
                        future.get();
                }
        }

public:
        BvhBuild(const std::span<BvhObject<N, T>>& objects)
        {
                object_indices_.reserve(objects.size());

                nodes_.emplace_back();
                constexpr unsigned ROOT = 0;
                tasks_.push(objects, compute_bounds(objects), ROOT);

                run();
        }

        const std::vector<unsigned>& object_indices() const
        {
                return object_indices_;
        }

        const std::vector<BvhBuildNode<N, T>>& nodes() const
        {
                return nodes_;
        }
};
}
