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
R. Stuart Ferguson.
Practical Algorithms For 3D Computer Graphics, Second Edition.
CRC Press, 2014.

5.3.4 Octree decomposition
*/

#pragma once

#include "bounding_box.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/progression.h>
#include <src/com/spin_lock.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <list>
#include <mutex>
#include <numeric>
#include <stack>
#include <tuple>
#include <vector>

namespace ns::geometry
{
namespace spatial_subdivision_tree_implementation
{
template <std::size_t DIMENSION>
inline constexpr int BOX_COUNT = 1u << DIMENSION;

template <typename Parallelotope>
class Box final
{
        static constexpr int EMPTY = -1;

        static constexpr int CHILD_COUNT = BOX_COUNT<Parallelotope::SPACE_DIMENSION>;

        Parallelotope parallelotope_;
        std::vector<int> object_indices_;
        std::array<int, CHILD_COUNT> childs_ = make_array_value<int, CHILD_COUNT>(EMPTY);

public:
        explicit Box(Parallelotope&& parallelotope) : parallelotope_(std::move(parallelotope))
        {
        }

        Box(Parallelotope&& parallelotope, std::vector<int>&& object_indices)
                : parallelotope_(std::move(parallelotope)), object_indices_(std::move(object_indices))
        {
        }

        const Parallelotope& parallelotope() const
        {
                return parallelotope_;
        }

        void set_child(const int child_number, const int child_box_index)
        {
                childs_[child_number] = child_box_index;
        }

        const std::array<int, CHILD_COUNT>& childs() const
        {
                return childs_;
        }

        bool has_childs() const
        {
                // all are empty or all are not empty
                return childs_[0] != EMPTY;
        }

        void add_object_index(const int object_index)
        {
                object_indices_.push_back(object_index);
        }

        void shrink_objects()
        {
                object_indices_.shrink_to_fit();
        }

        const std::vector<int>& object_indices() const
        {
                return object_indices_;
        }

        int object_index_count() const
        {
                return object_indices_.size();
        }

        void delete_all_objects()
        {
                object_indices_.clear();
                object_indices_.shrink_to_fit();
        }
};

inline std::vector<int> zero_based_indices(const int object_index_count)
{
        std::vector<int> object_indices(object_index_count);
        std::iota(object_indices.begin(), object_indices.end(), 0);
        return object_indices;
}

template <template <typename...> typename Container, typename Parallelotope>
std::vector<Box<Parallelotope>> move_boxes_to_vector(Container<Box<Parallelotope>>&& boxes)
{
        std::vector<Box<Parallelotope>> vector;

        vector.reserve(boxes.size());
        for (Box<Parallelotope>& v : boxes)
        {
                vector.emplace_back(std::move(v));
        }
        vector.shrink_to_fit();

        for (Box<Parallelotope>& box : vector)
        {
                box.shrink_objects();
        }

        return vector;
}

template <typename Box>
class BoxJobs final
{
        // If there are no jobs and all thread do nothing, then there will be no more jobs.
        // If there are no jobs and a thread do something, then new jobs can be created.
        // Instead of counting jobs for each thread, the sum of jobs across all threads is used.
        // A thread requires a new jobs without having a job - the sum is the same.
        // A thread requires a new jobs having a job - the sum decreases by 1.
        // A thread gets a new jobs - the sum increases by 1.
        int job_count_ = 0;

        std::stack<std::tuple<Box*, int>, std::vector<std::tuple<Box*, int>>> jobs_;
        SpinLock lock_;

        bool stop_all_ = false;

public:
        BoxJobs(Box* const box, const int depth) : jobs_({{box, depth}})
        {
        }

        void stop_all()
        {
                std::lock_guard lg(lock_);

                stop_all_ = true;
        }

        void push(Box* box, const int depth)
        {
                std::lock_guard lg(lock_);

                jobs_.emplace(box, depth);
        }

        bool pop(Box** const box, int* const depth)
        {
                std::lock_guard lg(lock_);

                if (stop_all_)
                {
                        return false;
                }

                if (*box)
                {
                        --job_count_;
                }

                if (!jobs_.empty())
                {
                        std::tie(*box, *depth) = jobs_.top();
                        jobs_.pop();
                        ++job_count_;
                        return true;
                }

                if (job_count_ > 0)
                {
                        *box = nullptr;
                        return true;
                }

                return false;
        }
};

template <template <typename...> typename Container, typename Parallelotope, int... I>
std::array<std::tuple<int, Box<Parallelotope>*, int>, BOX_COUNT<Parallelotope::SPACE_DIMENSION>> create_child_boxes(
        SpinLock* const boxes_lock,
        Container<Box<Parallelotope>>* const boxes,
        const Parallelotope& parallelotope,
        std::integer_sequence<int, I...>)
{
        static_assert(BOX_COUNT<Parallelotope::SPACE_DIMENSION> == sizeof...(I));
        static_assert(((I >= 0 && I < sizeof...(I)) && ...));

        std::array<Parallelotope, sizeof...(I)> child_parallelotopes = parallelotope.binary_division();

        std::lock_guard lg(*boxes_lock);

        int index = boxes->size();

        return {std::make_tuple(I, &(boxes->emplace_back(std::move(child_parallelotopes[I]))), index++)...};
}

template <template <typename...> typename Container, typename Parallelotope, typename ObjectIntersections>
void extend(
        const int MAX_DEPTH,
        const int MIN_OBJECTS,
        const int MAX_BOXES,
        SpinLock* const boxes_lock,
        Container<Box<Parallelotope>>* const boxes,
        BoxJobs<Box<Parallelotope>>* const box_jobs,
        const ObjectIntersections& object_intersections,
        ProgressRatio* const progress)
try
{
        static_assert(
                (std::is_same_v<Container<Box<Parallelotope>>, std::deque<Box<Parallelotope>>>)
                || (std::is_same_v<Container<Box<Parallelotope>>, std::list<Box<Parallelotope>>>));

        constexpr auto integer_sequence_n =
                std::make_integer_sequence<int, BOX_COUNT<Parallelotope::SPACE_DIMENSION>>();

        Box<Parallelotope>* box = nullptr; // no previous job
        int depth;

        while (box_jobs->pop(&box, &depth))
        {
                if (!box)
                {
                        continue;
                }

                if (depth >= MAX_DEPTH || box->object_index_count() <= MIN_OBJECTS)
                {
                        continue;
                }

                for (const auto& [i, child_box, child_box_index] :
                     create_child_boxes(boxes_lock, boxes, box->parallelotope(), integer_sequence_n))
                {
                        box->set_child(i, child_box_index);

                        if ((child_box_index & 0xfff) == 0xfff)
                        {
                                progress->set(child_box_index, MAX_BOXES);
                        }

                        for (int object_index : object_intersections(child_box->parallelotope(), box->object_indices()))
                        {
                                child_box->add_object_index(object_index);
                        }

                        box_jobs->push(child_box, depth + 1);
                }

                box->delete_all_objects();
        }
}
catch (...)
{
        box_jobs->stop_all();
        throw;
}

inline double maximum_box_count(const int box_count, const int max_depth)
{
        return geometric_progression_sum(box_count, max_depth);
}
}

template <typename Parallelotope>
class SpatialSubdivisionTree final
{
        using Box = spatial_subdivision_tree_implementation::Box<Parallelotope>;
        using BoxJobs = spatial_subdivision_tree_implementation::BoxJobs<Box>;

        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        // std::deque or std::list to keep object addreses unchanged when inserting
        using BoxContainer = std::deque<Box>;

        static constexpr T GUARD_REGION_SIZE = 1e-4;

        static constexpr int MIN_OBJECTS_PER_BOX_MIN = 2;
        static constexpr int MIN_OBJECTS_PER_BOX_MAX = 100;

        static constexpr int MAX_DEPTH = 10;

        static constexpr int BOX_COUNT_LIMIT = (1u << 31) - 1;

        static constexpr int RAY_OFFSET_IN_EPSILONS = 10;

        static constexpr int BOX_COUNT_SUBDIVISION = spatial_subdivision_tree_implementation::BOX_COUNT<N>;

        static constexpr int ROOT_BOX = 0;

        std::vector<Box> boxes_;

        T ray_offset_;

        const Box* find_box_for_point(const Box& box, const Vector<N, T>& p) const
        {
                if (!box.parallelotope().inside(p))
                {
                        return nullptr;
                }

                if (!box.has_childs())
                {
                        return &box;
                }

                for (int child_box : box.childs())
                {
                        const Box* b = find_box_for_point(boxes_[child_box], p);
                        if (b)
                        {
                                return b;
                        }
                }

                return nullptr;
        }

        const Box* find_box_for_point(const Vector<N, T>& p) const
        {
                return find_box_for_point(boxes_[ROOT_BOX], p);
        }

public:
        template <typename ObjectIntersections>
        void decompose(
                const int max_depth,
                const int min_objects_per_box,
                const int object_count,
                const BoundingBox<N, T>& bounding_box,
                const ObjectIntersections& object_intersections,
                const unsigned thread_count,
                ProgressRatio* const progress)

        {
                static_assert(BOX_COUNT_LIMIT <= (1LL << 31) - 1);

                namespace impl = spatial_subdivision_tree_implementation;

                if (!(max_depth >= 1 && max_depth <= MAX_DEPTH)
                    || !(min_objects_per_box >= MIN_OBJECTS_PER_BOX_MIN
                         && min_objects_per_box <= MIN_OBJECTS_PER_BOX_MAX))
                {
                        error("Error limits for spatial subdivision " + to_string(BOX_COUNT_SUBDIVISION)
                              + "-tree. Maximum depth (" + to_string(max_depth) + ") must be in the interval [1, "
                              + to_string(MAX_DEPTH) + "] and minimum objects per box ("
                              + to_string(min_objects_per_box) + ") must be in the interval ["
                              + to_string(MIN_OBJECTS_PER_BOX_MIN) + ", " + to_string(MIN_OBJECTS_PER_BOX_MAX) + "].");
                }

                if (impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth) > BOX_COUNT_LIMIT + 0.1)
                {
                        error("Spatial subdivision " + to_string(BOX_COUNT_SUBDIVISION) + "-tree is too deep. Depth "
                              + to_string(max_depth) + ", maximum box count "
                              + to_string(impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth))
                              + ", maximum box count limit " + to_string(BOX_COUNT_LIMIT));
                }

                const Vector<N, T> guard_region(GUARD_REGION_SIZE * (bounding_box.max - bounding_box.min).norm());
                const BoundingBox<N, T> root(bounding_box.min - guard_region, bounding_box.max + guard_region);

                ray_offset_ = std::max(root.max.norm_infinity(), root.min.norm_infinity())
                              * (RAY_OFFSET_IN_EPSILONS * limits<T>::epsilon() * std::sqrt(T(N)));

                const int max_box_count = std::lround(impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth));

                BoxContainer boxes;
                boxes.emplace_back(Parallelotope(root.min, root.max), impl::zero_based_indices(object_count));

                BoxJobs jobs(&boxes.front(), 1 /*depth*/);

                SpinLock boxes_lock;

                ThreadsWithCatch threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add(
                                [&]()
                                {
                                        extend(max_depth, min_objects_per_box, max_box_count, &boxes_lock, &boxes,
                                               &jobs, object_intersections, progress);
                                });
                }
                threads.join();

                boxes_ = move_boxes_to_vector(std::move(boxes));
        }

        const Parallelotope& root() const
        {
                return boxes_[ROOT_BOX].parallelotope();
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return boxes_[ROOT_BOX].parallelotope().intersect_volume(ray);
        }

        // this function is called after intersect_root.
        // root_t is the intersection found by intersect_root.
        template <typename FindIntersection>
        bool trace_ray(Ray<N, T> ray, const T root_t, const FindIntersection& find_intersection) const
        {
                const Box* box;
                Vector<N, T> point;

                point = ray.point(root_t);
                ray.set_org(point);
                box = find_box_for_point(point);
                if (!box)
                {
                        box = find_box_for_point(ray.point(ray_offset_));
                        if (!box)
                        {
                                return false;
                        }
                }

                while (true)
                {
                        if (box->object_index_count() > 0)
                        {
                                std::optional<Vector<N, T>> intersection = find_intersection(box->object_indices());
                                if (intersection && box->parallelotope().inside(*intersection))
                                {
                                        return true;
                                }
                        }

                        std::optional<T> next = box->parallelotope().intersect_farthest(ray);
                        if (!next)
                        {
                                next = 0;
                        }

                        ray.set_org(ray.point(*next));
                        for (T i = 1, offset = ray_offset_;; i *= 2, offset = i * ray_offset_)
                        {
                                point = ray.point(offset);
                                const Box* next_box = find_box_for_point(point);
                                if (!next_box)
                                {
                                        return false;
                                }
                                if (next_box != box)
                                {
                                        box = next_box;
                                        ray.set_org(point);
                                        break;
                                }
                                if (i >= T(1e10))
                                {
                                        return false;
                                }
                        }
                }
        }
};
}
