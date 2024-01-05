/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "tree.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/com/progression.h>
#include <src/com/thread.h>
#include <src/com/thread_tasks.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace ns::geometry::accelerators
{
namespace
{
template <std::size_t N>
constexpr int BOX_COUNT = 1u << N;

template <typename T>
constexpr T GUARD_REGION_SIZE = 1e-4;

constexpr int MIN_OBJECTS_PER_BOX = 10;
constexpr int MAX_DEPTH = 10;
constexpr int BOX_COUNT_LIMIT = (1u << 31) - 1;
constexpr int RAY_OFFSET_IN_EPSILONS = 10;

template <std::size_t N>
unsigned tree_max_depth()
{
        static_assert(N >= 3);

        switch (N)
        {
        case 3:
                return 10;
        case 4:
                return 8;
        case 5:
                return 6;
        case 6:
                return 5;
        default:
                static constexpr double SUM = 1e9;
                static constexpr double RATIO = power<N>(2);
                const double n = geometric_progression_n(RATIO, SUM);
                return std::max<unsigned>(2, std::floor(n));
        }
}

std::vector<int> zero_based_indices(const int count)
{
        std::vector<int> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.push_back(i);
        }
        return res;
}

double maximum_box_count(const unsigned box_count, const unsigned max_depth)
{
        return geometric_progression_sum(box_count, max_depth);
}

template <typename Box>
struct ChildBox final
{
        Box* box;
        int index;
};

template <typename Box, typename Parallelotope>
std::array<ChildBox<Box>, BOX_COUNT<Parallelotope::SPACE_DIMENSION>> create_child_boxes(
        const Parallelotope& parallelotope,
        std::mutex* const boxes_lock,
        std::deque<Box>* const boxes)
{
        constexpr int N = BOX_COUNT<Parallelotope::SPACE_DIMENSION>;
        std::array<Parallelotope, N> child_parallelotopes = parallelotope.binary_division();
        std::array<ChildBox<Box>, N> res;

        const std::lock_guard lg(*boxes_lock);
        for (int i = 0, index = boxes->size(); i < N; ++i, ++index)
        {
                res[i].box = &boxes->emplace_back(std::move(child_parallelotopes[i]));
                res[i].index = index;
        }
        return res;
}

template <typename Box>
struct Task final
{
        Box* box;
        unsigned depth;

        Task(Box* const box, const unsigned depth)
                : box(box),
                  depth(depth)
        {
        }
};

template <typename Box, typename Objects>
void extend(
        const unsigned max_depth,
        const unsigned min_objects,
        const unsigned max_boxes,
        std::mutex* const boxes_lock,
        std::deque<Box>* const boxes,
        ThreadTaskManager<Task<Box>>* const task_manager,
        const Objects& objects,
        progress::Ratio* const progress)
{
        while (const auto task = task_manager->get())
        {
                if (task->depth >= max_depth || task->box->object_indices.size() <= min_objects)
                {
                        task->box->childs[0] = -1;
                        continue;
                }

                const std::array child_boxes = create_child_boxes(task->box->parallelotope, boxes_lock, boxes);
                for (std::size_t i = 0; i < child_boxes.size(); ++i)
                {
                        if ((child_boxes[i].index & 0xfff) == 0xfff)
                        {
                                progress->set(child_boxes[i].index, max_boxes);
                        }

                        task->box->childs[i] = child_boxes[i].index;

                        Box* const child_box = child_boxes[i].box;

                        {
                                const std::vector<int> indices = objects.intersection_indices(
                                        child_box->parallelotope, task->box->object_indices);
                                child_box->object_indices.reserve(indices.size());
                                for (const int index : indices)
                                {
                                        child_box->object_indices.push_back(index);
                                }
                        }

                        task_manager->emplace(child_box, task->depth + 1);
                }

                task->box->object_indices.clear();
                task->box->object_indices.shrink_to_fit();
        }
}

template <std::size_t N>
void check_max_depth(const int max_depth)
{
        if (!(max_depth >= 1 && max_depth <= MAX_DEPTH))
        {
                error("Error limits for spatial subdivision " + to_string(BOX_COUNT<N>) + "-tree. Maximum depth ("
                      + to_string(max_depth) + ") must be in the interval [1, " + to_string(MAX_DEPTH) + "].");
        }

        if (!(maximum_box_count(BOX_COUNT<N>, max_depth) <= BOX_COUNT_LIMIT))
        {
                error("Spatial subdivision " + to_string(BOX_COUNT<N>) + "-tree is too deep. Depth "
                      + to_string(max_depth) + ", maximum box count "
                      + to_string(maximum_box_count(BOX_COUNT<N>, max_depth)) + ", maximum box count limit "
                      + to_string(BOX_COUNT_LIMIT));
        }
}
}

template <typename Parallelotope>
SpatialSubdivisionTree<Parallelotope>::SpatialSubdivisionTree(const Objects& objects, progress::Ratio* const progress)
{
        static_assert(BOX_COUNT_LIMIT <= (1LL << 31) - 1);

        const int max_depth = tree_max_depth<N>();

        check_max_depth<N>(max_depth);

        const Vector<N, T> guard_region{GUARD_REGION_SIZE<T> * objects.bounding_box().diagonal().norm()};
        const spatial::BoundingBox<N, T> root{
                objects.bounding_box().min() - guard_region, objects.bounding_box().max() + guard_region};
        const int max_box_count = std::lround(maximum_box_count(BOX_COUNT<N>, max_depth));

        std::mutex boxes_lock;

        // std::deque to keep object addreses unchanged when inserting
        std::deque<Box> boxes;
        boxes.emplace_back(Parallelotope(root.min(), root.max()));
        boxes.back().object_indices = zero_based_indices(objects.count());

        ThreadTasks<Task<Box>> tasks;
        tasks.emplace(&boxes.front(), 1 /*depth*/);

        const auto f = [&]()
        {
                try
                {
                        ThreadTaskManager<Task<Box>> task_manager(&tasks);
                        extend(max_depth, MIN_OBJECTS_PER_BOX, max_box_count, &boxes_lock, &boxes, &task_manager,
                               objects, progress);
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

        boxes_.reserve(boxes.size());
        for (Box& v : boxes)
        {
                boxes_.push_back(std::move(v));
        }

        ray_offset_ = std::max(root.min().norm_infinity(), root.max().norm_infinity())
                      * (RAY_OFFSET_IN_EPSILONS * Limits<T>::epsilon() * std::sqrt(T{N}));
}

#define TEMPLATE(N, T) template class SpatialSubdivisionTree<spatial::ParallelotopeAA<(N), T>>;

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
