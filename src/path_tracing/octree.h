/*
Copyright (C) 2017 Topological Manifold

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

// R. Stuart Ferguson.
// Practical Algorithms For 3D Computer Graphics, Second Edition.
// CRC Press, 2014.
// Раздел 5.3.4 Octree decomposition.

#pragma once

#include "constants.h"
#include "ray.h"

#include "com/arrays.h"
#include "com/error.h"
#include "com/thread.h"
#include "com/vec.h"
#include "progress/progress.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <list>
#include <stack>
#include <tuple>
#include <vector>

namespace OctreeImplementation
{
template <typename Parallelepiped, typename OctreeObject>
class OctreeBox
{
        static constexpr int EMPTY = -1;

        Parallelepiped m_parallelepiped;
        std::vector<const OctreeObject*> m_objects;
        std::array<int, 8> m_childs = make_array_value<int, 8>(EMPTY);

public:
        OctreeBox(Parallelepiped&& box) : m_parallelepiped(std::move(box))
        {
        }

        const Parallelepiped& get_parallelepiped() const
        {
                return m_parallelepiped;
        }

        void set_child(int child_number, int child_box_index)
        {
                m_childs[child_number] = child_box_index;
        }

        const std::array<int, 8>& get_childs() const
        {
                return m_childs;
        }

        bool has_childs() const
        {
                // Они или все заполнены, или все не заполнены
                return m_childs[0] != EMPTY;
        }

        void add_object(const OctreeObject* obj)
        {
                m_objects.push_back(obj);
        }

        void set_all_objects(std::vector<const OctreeObject*>&& objects)
        {
                m_objects = std::move(objects);
        }

        void shrink_objects()
        {
                m_objects.shrink_to_fit();
        }

        const std::vector<const OctreeObject*>& get_objects() const
        {
                return m_objects;
        }

        int get_object_count() const
        {
                return m_objects.size();
        }

        void delete_all_objects()
        {
                m_objects.clear();
                m_objects.shrink_to_fit();
        }
};

template <typename T>
std::vector<const T*> pointers_of_objects(const std::vector<T>& objects)
{
        std::vector<const T*> object_pointers;
        object_pointers.reserve(objects.size());
        for (const T& object : objects)
        {
                object_pointers.push_back(&object);
        }
        return object_pointers;
}

template <template <typename...> typename Container, typename Parallelepiped, typename OctreeObject>
std::vector<OctreeBox<Parallelepiped, OctreeObject>> move_boxes_from_container_to_vector_and_shrink(
        Container<OctreeBox<Parallelepiped, OctreeObject>>&& boxes)
{
        std::vector<OctreeBox<Parallelepiped, OctreeObject>> vector;

        vector.reserve(boxes.size());
        for (OctreeBox<Parallelepiped, OctreeObject>& v : boxes)
        {
                vector.emplace_back(std::move(v));
        }
        vector.shrink_to_fit();
        for (OctreeBox<Parallelepiped, OctreeObject>& box : vector)
        {
                box.shrink_objects();
        }

        return vector;
}

template <typename Parallelepiped, typename OctreeObject, typename ConvexHullVertices>
Parallelepiped cuboid_of_objects(const std::vector<OctreeObject>& objects, const ConvexHullVertices& functor_convex_hull_vertices)
{
        // Прямоугольный параллелепипед, параллельный координатным плоскостям

        constexpr double MAX = std::numeric_limits<double>::max();
        constexpr double MIN = std::numeric_limits<double>::lowest();

        vec3 min(MAX, MAX, MAX);
        vec3 max(MIN, MIN, MIN);

        for (const OctreeObject& object : objects)
        {
                for (const vec3& v : functor_convex_hull_vertices(object))
                {
                        for (int i = 0; i < 3; ++i)
                        {
                                min[i] = std::min(v[i], min[i]);
                                max[i] = std::max(v[i], max[i]);
                        }
                }
        }

        if (!(min[0] < max[0] && min[1] < max[1] && min[2] < max[2]))
        {
                error("Objects for octree don't form 3D object");
        }

        vec3 d = max - min;

        return Parallelepiped(min, vec3(d[0], 0, 0), vec3(0, d[1], 0), vec3(0, 0, d[2]));
}

template <typename Box>
class BoxJobs
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

        void stop_all() noexcept
        {
                std::lock_guard lg(m_lock);

                m_stop_all = true;
        }

        void push(Box* box, int depth) noexcept
        {
                std::lock_guard lg(m_lock);

                m_jobs.emplace(box, depth);
        }

        bool pop(Box** box, int* depth) noexcept
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
}

template <typename Parallelepiped, typename OctreeObject>
class Octree
{
        using OctreeBox = OctreeImplementation::OctreeBox<Parallelepiped, OctreeObject>;
        using BoxJobs = OctreeImplementation::BoxJobs<OctreeBox>;

        // Нужно, чтобы указатели не менялись при вставке элементов,
        // поэтому std::list или std::deque.
        template <typename T>
        using BoxContainer = std::list<T>;

        // Смещение по лучу внутрь коробки от точки персечения с коробкой.
        static constexpr double DELTA = 10 * INTERSECTION_THRESHOLD;

        // Нижняя и верхняя границы для минимального количества объектов в коробке.
        static constexpr int MIN_OBJECTS_LEFT_BOUND = 2;
        static constexpr int MIN_OBJECTS_RIGHT_BOUND = 100;

        // Нижняя и верхняя границы для глубины дерева.
        static constexpr int MAX_DEPTH_LEFT_BOUND = 1;
        static constexpr int MAX_DEPTH_RIGHT_BOUND = 10;

        // Первым элементом массива является только 0.
        static constexpr int ROOT_BOX = 0;

        // Максимальная глубина и минимальное количество объектов для экземпляра дерева.
        const int MAX_DEPTH, MIN_OBJECTS;
        // Максимальное количество коробок — сумма геометрической прогресии со знаменателем 8.
        // Требуется для того, чтобы что-то отображать как расчёт.
        const int MAX_BOXES = (std::pow(8, MAX_DEPTH) - 1) / (8 - 1);

        // Все коробки хранятся в одном векторе
        std::vector<OctreeBox> m_boxes;

        template <template <typename...> typename Container>
        static std::array<std::tuple<OctreeBox*, int>, 8> create_boxes(SpinLock* boxes_lock, Container<OctreeBox>* boxes,
                                                                       std::array<Parallelepiped, 8>&& parallelepipeds)
        {
                std::array<std::tuple<OctreeBox*, int>, 8> res;

                std::lock_guard lg(*boxes_lock);

                for (int i = 0, index = boxes->size(); i < 8; ++i, ++index)
                {
                        res[i] = std::make_tuple(&(boxes->emplace_back(std::move(parallelepipeds[i]))), index);
                }

                return res;
        }

        template <template <typename...> typename Container, typename ShapeIntersection>
        void extend(SpinLock* boxes_lock, Container<OctreeBox>* boxes, BoxJobs* box_jobs,
                    const ShapeIntersection& functor_shape_intersection, ProgressRatio* progress) const try
        {
                OctreeBox* box = nullptr; // nullptr — предыдущей задачи нет.
                int depth;

                while (box_jobs->pop(&box, &depth))
                {
                        if (!box)
                        {
                                // Новой задачи нет, но другие потоки работают над задачами.
                                continue;
                        }

                        if (depth >= MAX_DEPTH || box->get_object_count() <= MIN_OBJECTS)
                        {
                                continue;
                        }

                        std::array<std::tuple<OctreeBox*, int>, 8> child_boxes =
                                create_boxes(boxes_lock, boxes, box->get_parallelepiped().binary_division());

                        for (int i = 0; i < 8; ++i)
                        {
                                auto[child_box, child_box_index] = child_boxes[i];

                                box->set_child(i, child_box_index);

                                if (child_box_index & 0xfff)
                                {
                                        progress->set(child_box_index, MAX_BOXES);
                                }

                                for (const OctreeObject* obj : box->get_objects())
                                {
                                        if (functor_shape_intersection(&child_box->get_parallelepiped(), obj))
                                        {
                                                child_box->add_object(obj);
                                        }
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

        bool find_box_for_point(const OctreeBox& box, const vec3& p, const OctreeBox** found_box) const
        {
                if (!box.get_parallelepiped().inside(p))
                {
                        return false;
                }

                if (!box.has_childs())
                {
                        *found_box = &box;
                        return true;
                }

                for (int child_box : box.get_childs())
                {
                        if (find_box_for_point(m_boxes[child_box], p, found_box))
                        {
                                return true;
                        }
                }

                return false;
        }

public:
        Octree(int max_depth, int min_objects_per_box) : MAX_DEPTH(max_depth), MIN_OBJECTS(min_objects_per_box)
        {
        }

        template <typename ConvexHullVertices, typename ShapeIntersection>
        Octree(int max_depth, int min_objects_per_box, const std::vector<OctreeObject>& objects,
               const ConvexHullVertices& functor_convex_hull_vertices, const ShapeIntersection& functor_shape_intersection,
               unsigned decomposition_thread_count, ProgressRatio* progress)
                : MAX_DEPTH(max_depth), MIN_OBJECTS(min_objects_per_box)
        {
                decompose(objects, functor_convex_hull_vertices, functor_shape_intersection, decomposition_thread_count,
                          progress);
        }

        template <typename ConvexHullVertices, typename ShapeIntersection>
        void decompose(const std::vector<OctreeObject>& objects, const ConvexHullVertices& functor_convex_hull_vertices,
                       const ShapeIntersection& functor_shape_intersection, unsigned decomposition_thread_count,
                       ProgressRatio* progress)

        {
                if (!(MAX_DEPTH >= MAX_DEPTH_LEFT_BOUND && MAX_DEPTH <= MAX_DEPTH_RIGHT_BOUND) ||
                    !(MIN_OBJECTS >= MIN_OBJECTS_LEFT_BOUND && MIN_OBJECTS <= MIN_OBJECTS_RIGHT_BOUND))
                {
                        error("Error limits for octree");
                }

                using namespace OctreeImplementation;

                SpinLock boxes_lock;
                BoxContainer<OctreeBox> boxes;

                boxes.emplace_back(cuboid_of_objects<Parallelepiped>(objects, functor_convex_hull_vertices));
                boxes.begin()->set_all_objects(pointers_of_objects(objects));

                BoxJobs jobs(&*(boxes.begin()), MAX_DEPTH_LEFT_BOUND);

                std::vector<std::thread> threads(decomposition_thread_count);
                std::vector<std::string> msg(threads.size());
                for (unsigned i = 0; i < threads.size(); ++i)
                {
                        launch_thread(&threads[i], &msg[i],
                                      [&]() { extend(&boxes_lock, &boxes, &jobs, functor_shape_intersection, progress); });
                }
                join_threads(&threads, &msg);

                m_boxes = move_boxes_from_container_to_vector_and_shrink(std::move(boxes));
        }

        bool intersect_root(const ray3& ray, double* t) const
        {
                return m_boxes[ROOT_BOX].get_parallelepiped().intersect(ray, t);
        }

        // Вызывается после intersect_root. Если в intersect_root пересечение было найдено,
        // то сюда передаётся результат пересечения в параметре root_t.
        template <typename FindIntersection>
        bool trace_ray(ray3 ray, double root_t, const FindIntersection& functor_find_intersection) const
        {
                bool first = true;

                while (true)
                {
                        double t;
                        const OctreeBox* box;

                        if (find_box_for_point(m_boxes[ROOT_BOX], ray.get_org(), &box))
                        {
                                if (box->get_objects().size() > 0 &&
                                    functor_find_intersection(box->get_parallelepiped(), box->get_objects()))
                                {
                                        return true;
                                }

                                // Поиск пересечения с границей этого параллелепипеда
                                // для перехода в соседний параллелепипед.
                                if (!box->get_parallelepiped().intersect(ray, &t))
                                {
                                        return false;
                                }
                        }
                        else
                        {
                                // Начало луча не находится в пределах октадерева.

                                if (!first)
                                {
                                        // Не первый проход — процесс вышел за пределы октадерева.
                                        return false;
                                }
                                else
                                {
                                        // Первый проход — начало луча находится снаружи и надо искать
                                        // пересечение с самим октадеревом. Это пересечение уже должно
                                        // быть найдено ранее при вызове intersect_root.
                                        t = root_t;
                                }
                        }

                        ray.set_org(ray.point(t + DELTA));

                        first = false;
                }
        }
};
