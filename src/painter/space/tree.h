/*
Copyright (C) 2017-2020 Topological Manifold

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

 Раздел 5.3.4 Octree decomposition.
*/

#pragma once

#include "bounding_box.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
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
#include <numeric>
#include <stack>
#include <tuple>
#include <vector>

namespace painter
{
namespace spatial_subdivision_tree_implementation
{
template <size_t DIMENSION>
inline constexpr int BOX_COUNT = 1u << DIMENSION;

template <typename Parallelotope>
class Box final
{
        static constexpr int EMPTY = -1;

        static constexpr int CHILD_COUNT = BOX_COUNT<Parallelotope::SPACE_DIMENSION>;

        Parallelotope m_parallelotope;
        std::vector<int> m_object_indices;
        std::array<int, CHILD_COUNT> m_childs = make_array_value<int, CHILD_COUNT>(EMPTY);

public:
        explicit Box(Parallelotope&& parallelotope) : m_parallelotope(std::move(parallelotope))
        {
        }

        Box(Parallelotope&& parallelotope, std::vector<int>&& object_indices)
                : m_parallelotope(std::move(parallelotope)), m_object_indices(std::move(object_indices))
        {
        }

        const Parallelotope& parallelotope() const
        {
                return m_parallelotope;
        }

        void set_child(int child_number, int child_box_index)
        {
                m_childs[child_number] = child_box_index;
        }

        const std::array<int, CHILD_COUNT>& childs() const
        {
                return m_childs;
        }

        bool has_childs() const
        {
                // Они или все заполнены, или все не заполнены
                return m_childs[0] != EMPTY;
        }

        void add_object_index(int object_index)
        {
                m_object_indices.push_back(object_index);
        }

        void shrink_objects()
        {
                m_object_indices.shrink_to_fit();
        }

        const std::vector<int>& object_indices() const
        {
                return m_object_indices;
        }

        int object_index_count() const
        {
                return m_object_indices.size();
        }

        void delete_all_objects()
        {
                m_object_indices.clear();
                m_object_indices.shrink_to_fit();
        }
};

inline std::vector<int> zero_based_indices(int object_index_count)
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
        // Если задач нет, и все потоки с ними не работают, то всё сделано. Если задач нет,
        // но хотя бы один поток с ними работает, то могут появиться новые задачи и надо подождать.
        // Вместо учёта заданий для каждого потока, используется просто сумма задач по всем потокам:
        //   поток пришёл за новой задачей, не имея предыдущей — сумма не меняется;
        //   поток пришёл за новой задачей, имея предыдущую — уменьшили сумму на 1;
        //   дали задачу потоку — увеличили сумму на 1.
        int m_job_count = 0;

        std::stack<std::tuple<Box*, int>, std::vector<std::tuple<Box*, int>>> m_jobs;
        SpinLock m_lock;

        bool m_stop_all = false;

public:
        BoxJobs(Box* box, int depth) : m_jobs({{box, depth}})
        {
        }

        void stop_all()
        {
                std::lock_guard lg(m_lock);

                m_stop_all = true;
        }

        void push(Box* box, int depth)
        {
                std::lock_guard lg(m_lock);

                m_jobs.emplace(box, depth);
        }

        bool pop(Box** box, int* depth)
        {
                std::lock_guard lg(m_lock);

                if (m_stop_all)
                {
                        return false;
                }

                if (*box)
                {
                        --m_job_count;
                }

                if (!m_jobs.empty())
                {
                        std::tie(*box, *depth) = m_jobs.top();
                        m_jobs.pop();
                        ++m_job_count;
                        return true;
                }

                if (m_job_count > 0)
                {
                        // Заданий нет, но какие-то потоки ещё работают,
                        // поэтому могут появиться новые задания.
                        *box = nullptr;
                        return true;
                }

                // Заданий нет и все потоки с ними не работают.
                return false;
        }
};

template <template <typename...> typename Container, typename Parallelotope, int... I>
std::array<std::tuple<int, Box<Parallelotope>*, int>, BOX_COUNT<Parallelotope::SPACE_DIMENSION>> create_child_boxes(
        SpinLock* boxes_lock,
        Container<Box<Parallelotope>>* boxes,
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
        SpinLock* boxes_lock,
        Container<Box<Parallelotope>>* boxes,
        BoxJobs<Box<Parallelotope>>* box_jobs,
        const ObjectIntersections& object_intersections,
        ProgressRatio* progress)
try
{
        // Адреса имеющихся элементов не должны меняться при вставке
        // новых элементов, поэтому требуется std::deque или std::list.
        static_assert(
                std::is_same_v<
                        Container<Box<Parallelotope>>,
                        std::deque<Box<
                                Parallelotope>>> || std::is_same_v<Container<Box<Parallelotope>>, std::list<Box<Parallelotope>>>);

        constexpr auto integer_sequence_n =
                std::make_integer_sequence<int, BOX_COUNT<Parallelotope::SPACE_DIMENSION>>();

        Box<Parallelotope>* box = nullptr; // nullptr — предыдущей задачи нет.
        int depth;

        while (box_jobs->pop(&box, &depth))
        {
                if (!box)
                {
                        // Новой задачи нет, но другие потоки работают над задачами.
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

inline double maximum_box_count(int box_count, int max_depth)
{
        // Сумма геометрической прогрессии со знаменателем box_count.
        // Sum = (pow(r, n) - 1) / (r - 1).

        return (std::pow(box_count, max_depth) - 1) / (box_count - 1);
}
}

template <typename Parallelotope>
class SpatialSubdivisionTree final
{
        using Box = spatial_subdivision_tree_implementation::Box<Parallelotope>;
        using BoxJobs = spatial_subdivision_tree_implementation::BoxJobs<Box>;

        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        // Адреса имеющихся элементов не должны меняться при вставке
        // новых элементов, поэтому требуется std::deque или std::list.
        using BoxContainer = std::deque<Box>;

        static constexpr T GUARD_REGION_SIZE = 1e-4;

        static constexpr int MIN_OBJECTS_PER_BOX_MIN = 2;
        static constexpr int MIN_OBJECTS_PER_BOX_MAX = 100;

        static constexpr int MAX_DEPTH = 10;

        static constexpr int BOX_COUNT_LIMIT = (1u << 31) - 1;

        static constexpr int BOX_COUNT_SUBDIVISION = spatial_subdivision_tree_implementation::BOX_COUNT<N>;

        // Первым элементом массива является только 0.
        static constexpr int ROOT_BOX = 0;

        std::vector<Box> m_boxes;

        Vector<N, T> m_distance_from_facet;

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
                        const Box* b = find_box_for_point(m_boxes[child_box], p);
                        if (b)
                        {
                                return b;
                        }
                }

                return nullptr;
        }

        const Box* find_box_for_point(const Vector<N, T>& p) const
        {
                return find_box_for_point(m_boxes[ROOT_BOX], p);
        }

public:
        template <typename ObjectIntersections>
        void decompose(
                int max_depth,
                int min_objects_per_box,
                int object_count,
                const BoundingBox<N, T>& bounding_box,
                const ObjectIntersections& object_intersections,
                unsigned thread_count,
                ProgressRatio* progress)

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

                // Немного прибавить к максимуму для учёта ошибок плавающей точки
                if (impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth) > BOX_COUNT_LIMIT + 0.1)
                {
                        error("Spatial subdivision " + to_string(BOX_COUNT_SUBDIVISION) + "-tree is too deep. Depth "
                              + to_string(max_depth) + ", maximum box count "
                              + to_string(impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth))
                              + ", maximum box count limit " + to_string(BOX_COUNT_LIMIT));
                }

                const Vector<N, T> guard_region(GUARD_REGION_SIZE * (bounding_box.max - bounding_box.min).norm());
                const BoundingBox<N, T> root(bounding_box.min - guard_region, bounding_box.max + guard_region);

                // Максимальное разделение по одной координате
                const int max_divisions = 1u << (max_depth - 1);

                for (unsigned i = 0; i < N; ++i)
                {
                        m_distance_from_facet[i] = (root.max[i] - root.min[i]) / max_divisions / 2;
                }

                const int max_box_count = std::lround(impl::maximum_box_count(BOX_COUNT_SUBDIVISION, max_depth));

                BoxContainer boxes;
                boxes.emplace_back(Parallelotope(root.min, root.max), impl::zero_based_indices(object_count));

                BoxJobs jobs(&boxes.front(), 1 /*depth*/);

                SpinLock boxes_lock;

                ThreadsWithCatch threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add([&]() {
                                extend(max_depth, min_objects_per_box, max_box_count, &boxes_lock, &boxes, &jobs,
                                       object_intersections, progress);
                        });
                }
                threads.join();

                m_boxes = move_boxes_to_vector(std::move(boxes));
        }

        const Parallelotope& root() const
        {
                return m_boxes[ROOT_BOX].parallelotope();
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return m_boxes[ROOT_BOX].parallelotope().intersect_volume(ray);
        }

        // Вызывается после intersect_root. Если в intersect_root пересечение было найдено,
        // то сюда передаётся результат пересечения в параметре root_t.
        template <typename FindIntersection>
        bool trace_ray(Ray<N, T> ray, T root_t, const FindIntersection& find_intersection) const
        {
                const Box* box;
                Vector<N, T> point;

                point = ray.point(root_t);
                ray.set_org(point);
                box = find_box_for_point(point);
                if (!box)
                {
                        box = find_box_for_point(
                                point - m_distance_from_facet * m_boxes[ROOT_BOX].parallelotope().normal(point));
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
                                return false;
                        }

                        point = ray.point(*next);
                        ray.set_org(point);
                        box = find_box_for_point(point + m_distance_from_facet * box->parallelotope().normal(point));
                        if (!box)
                        {
                                return false;
                        }
                }
        }
};
}
