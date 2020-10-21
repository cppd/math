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

        static constexpr int CHILD_COUNT = BOX_COUNT<Parallelotope::DIMENSION>;

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

inline std::vector<int> iota_zero_based_indices(int object_index_count)
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

template <typename Parallelotope, size_t... I>
constexpr Parallelotope create_parallelotope_from_vector(
        const Vector<sizeof...(I), typename Parallelotope::DataType>& org,
        const Vector<sizeof...(I), typename Parallelotope::DataType>& d,
        std::integer_sequence<size_t, I...>)
{
        static_assert(Parallelotope::DIMENSION == sizeof...(I));

        using T = typename Parallelotope::DataType;
        constexpr int N = Parallelotope::DIMENSION;

        // Заполнение диагонали матрицы NxN значениями из d, остальные элементы 0
        std::array<Vector<N, T>, N> edges{(static_cast<void>(I), Vector<N, T>(0))...};
        ((edges[I][I] = d[I]), ...);

        return Parallelotope(org, edges[I]...);
}

template <size_t N, typename T, typename ObjectVertices>
void min_max_and_distance(
        int max_divisions,
        int distance_from_facet_in_epsilons,
        int object_index_count,
        const ObjectVertices& object_vertices,
        Vector<N, T>* minimum,
        Vector<N, T>* maximum,
        T* distance)
{
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> min(limits<T>::max());
        Vector<N, T> max(limits<T>::lowest());

        for (int object_index = 0; object_index < object_index_count; ++object_index)
        {
                for (const Vector<N, T>& v : object_vertices(object_index))
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                min[i] = std::min(v[i], min[i]);
                                max[i] = std::max(v[i], max[i]);
                        }
                }
        }

        for (unsigned i = 0; i < N; ++i)
        {
                if (!(min[i] < max[i]))
                {
                        error("Objects for (2^N)-tree don't form N-dimensional object");
                }
        }

        T all_max = limits<T>::lowest();

        for (unsigned i = 0; i < N; ++i)
        {
                T abs_max = std::max(std::abs(min[i]), std::abs(max[i]));

                T guard_region_size = abs_max * (distance_from_facet_in_epsilons * limits<T>::epsilon());
                min[i] -= guard_region_size;
                max[i] += guard_region_size;

                all_max = std::max(abs_max, all_max);
        }

        T dist = all_max * (distance_from_facet_in_epsilons * limits<T>::epsilon());

        for (unsigned i = 0; i < N; ++i)
        {
                T one_half_of_min_box_size = (max[i] - min[i]) / max_divisions / 2;
                if (dist >= one_half_of_min_box_size)
                {
                        error("The minimal distance from facets " + to_string(dist)
                              + " is greater than one half of the minimum box size "
                              + to_string(one_half_of_min_box_size) + " (dimension #" + to_string(i) + ")");
                }
        }

        *minimum = min;
        *maximum = max;
        *distance = dist;
}

template <typename Parallelotope>
Parallelotope root_parallelotope(
        const Vector<Parallelotope::DIMENSION, typename Parallelotope::DataType>& min,
        const Vector<Parallelotope::DIMENSION, typename Parallelotope::DataType>& max)
{
        static_assert(std::is_floating_point_v<typename Parallelotope::DataType>);

        const Vector<Parallelotope::DIMENSION, typename Parallelotope::DataType>& diagonal = max - min;

        return create_parallelotope_from_vector<Parallelotope>(
                min, diagonal, std::make_integer_sequence<size_t, Parallelotope::DIMENSION>());
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
std::array<std::tuple<int, Box<Parallelotope>*, int>, BOX_COUNT<Parallelotope::DIMENSION>> create_child_boxes(
        SpinLock* boxes_lock,
        Container<Box<Parallelotope>>* boxes,
        const Parallelotope& parallelotope,
        std::integer_sequence<int, I...>)
{
        static_assert(BOX_COUNT<Parallelotope::DIMENSION> == sizeof...(I));
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

        constexpr auto integer_sequence_n = std::make_integer_sequence<int, BOX_COUNT<Parallelotope::DIMENSION>>();

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
}

template <typename Parallelotope>
class SpatialSubdivisionTree final
{
        static double maximum_box_count(int box_count, int max_depth)
        {
                // Максимальное количество коробок — это сумма геометрической прогрессии
                // со знаменателем box_count.
                // Sum = (pow(r, n) - 1) / (r - 1).

                return (std::pow(box_count, max_depth) - 1) / (box_count - 1);
        }

        using Box = spatial_subdivision_tree_implementation::Box<Parallelotope>;
        using BoxJobs = spatial_subdivision_tree_implementation::BoxJobs<Box>;

        // Размерность задачи и тип данных
        static constexpr int N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        // Адреса имеющихся элементов не должны меняться при вставке
        // новых элементов, поэтому требуется std::deque или std::list.
        using BoxContainer = std::deque<Box>;

        // Расстояние от грани, при котором точка считается внутри коробки.
        static constexpr int DISTANCE_FROM_FACET_IN_EPSILONS = 20;

        // Нижняя и верхняя границы для минимального количества объектов в коробке.
        static constexpr int MIN_OBJECTS_LEFT_BOUND = 2;
        static constexpr int MIN_OBJECTS_RIGHT_BOUND = 100;

        // Нижняя и верхняя границы для глубины дерева.
        static constexpr int MAX_DEPTH_LEFT_BOUND = 1;
        static constexpr int MAX_DEPTH_RIGHT_BOUND = 10;

        // Первым элементом массива является только 0.
        static constexpr int ROOT_BOX = 0;

        // Максимально допустимое количество коробок.
        static constexpr int MAX_BOX_COUNT_LIMIT = (1u << 31) - 1;

        // Количество коробок при одном делении.
        static constexpr int BOX_COUNT = spatial_subdivision_tree_implementation::BOX_COUNT<N>;

        // Все коробки хранятся в одном векторе.
        std::vector<Box> m_boxes;

        T m_distance_from_facet;

        bool find_box_for_point(const Box& box, const Vector<N, T>& p, const Box** found_box) const
        {
                if (!box.parallelotope().inside(p))
                {
                        return false;
                }

                if (!box.has_childs())
                {
                        *found_box = &box;
                        return true;
                }

                for (int child_box : box.childs())
                {
                        if (find_box_for_point(m_boxes[child_box], p, found_box))
                        {
                                return true;
                        }
                }

                return false;
        }

public:
        template <typename ObjectVertices, typename ObjectIntersections>
        void decompose(
                int max_depth,
                int min_objects_per_box,
                int object_index_count,
                const ObjectVertices& object_vertices,
                const ObjectIntersections& object_intersections,
                unsigned thread_count,
                ProgressRatio* progress)

        {
                static_assert(MAX_BOX_COUNT_LIMIT <= (1LL << 31) - 1);

                namespace impl = spatial_subdivision_tree_implementation;

                if (!(max_depth >= MAX_DEPTH_LEFT_BOUND && max_depth <= MAX_DEPTH_RIGHT_BOUND)
                    || !(min_objects_per_box >= MIN_OBJECTS_LEFT_BOUND
                         && min_objects_per_box <= MIN_OBJECTS_RIGHT_BOUND))
                {
                        error("Error limits for spatial subdivision " + to_string(BOX_COUNT) + "-tree. Maximum depth ("
                              + to_string(max_depth) + ") must be in the interval [" + to_string(MAX_DEPTH_LEFT_BOUND)
                              + ", " + to_string(MAX_DEPTH_RIGHT_BOUND) + "] and minimum objects per box ("
                              + to_string(min_objects_per_box) + ") must be in the interval ["
                              + to_string(MIN_OBJECTS_LEFT_BOUND) + ", " + to_string(MIN_OBJECTS_RIGHT_BOUND) + "].");
                }

                // Немного прибавить к максимуму для учёта ошибок плавающей точки
                if (maximum_box_count(BOX_COUNT, max_depth) > MAX_BOX_COUNT_LIMIT + 0.1)
                {
                        error("Spatial subdivision " + to_string(BOX_COUNT) + "-tree is too deep. Depth "
                              + to_string(max_depth) + ", maximum box count "
                              + to_string(maximum_box_count(BOX_COUNT, max_depth)) + ", maximum box count limit "
                              + to_string(MAX_BOX_COUNT_LIMIT));
                }

                const int max_box_count = std::lround(maximum_box_count(BOX_COUNT, max_depth));

                // Максимальное разделение по одной координате
                const int max_divisions = 1u << (max_depth - 1);

                Vector<N, T> min;
                Vector<N, T> max;
                T distance_from_facet;

                impl::min_max_and_distance(
                        max_divisions, DISTANCE_FROM_FACET_IN_EPSILONS, object_index_count, object_vertices, &min, &max,
                        &distance_from_facet);

                BoxContainer boxes(
                        {Box(impl::root_parallelotope<Parallelotope>(min, max),
                             impl::iota_zero_based_indices(object_index_count))});

                BoxJobs jobs(&boxes.front(), MAX_DEPTH_LEFT_BOUND);

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
                m_distance_from_facet = distance_from_facet;
        }

        bool intersect_root(const Ray<N, T>& ray, T* t) const
        {
                return m_boxes[ROOT_BOX].parallelotope().intersect(ray, t);
        }

        // Вызывается после intersect_root. Если в intersect_root пересечение было найдено,
        // то сюда передаётся результат пересечения в параметре root_t.
        template <typename FindIntersection>
        bool trace_ray(Ray<N, T> ray, T root_t, const FindIntersection& find_intersection) const
        {
                bool first = true;

                Vector<N, T> interior_point = ray.org();

                while (true)
                {
                        T t;
                        const Box* box;

                        if (find_box_for_point(m_boxes[ROOT_BOX], interior_point, &box))
                        {
                                Vector<N, T> point;
                                if (box->object_index_count() > 0 && find_intersection(box->object_indices(), &point)
                                    && box->parallelotope().inside(point))
                                {
                                        return true;
                                }

                                // Поиск пересечения с дальней границей текущей коробки
                                // для перехода в соседнюю коробку.
                                if (!box->parallelotope().intersect_farthest(ray, &t))
                                {
                                        return false;
                                }

                                Vector<N, T> intersection_point = ray.point(t);
                                ray.set_org(intersection_point);
                                Vector<N, T> normal = box->parallelotope().normal(intersection_point);
                                interior_point = intersection_point + m_distance_from_facet * normal;
                        }
                        else
                        {
                                // Начало луча не находится в пределах дерева.

                                if (!first)
                                {
                                        // Не первый проход — процесс вышел за пределы дерева.
                                        return false;
                                }

                                // Первый проход — начало луча находится снаружи и надо искать
                                // пересечение с самим деревом. Это пересечение уже должно
                                // быть найдено ранее при вызове intersect_root.
                                Vector<N, T> intersection_point = ray.point(root_t);
                                ray.set_org(intersection_point);
                                Vector<N, T> normal = m_boxes[ROOT_BOX].parallelotope().normal(intersection_point);
                                interior_point = intersection_point - m_distance_from_facet * normal;
                        }

                        first = false;
                }
        }
};
}
