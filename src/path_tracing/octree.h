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

#include "com/error.h"
#include "com/vec.h"
#include "progress/progress.h"

#include <algorithm>
#include <array>
#include <cmath>
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
        std::array<int, 8> m_childs;

public:
        OctreeBox(Parallelepiped&& box) : m_parallelepiped(std::move(box))
        {
                std::fill(m_childs.begin(), m_childs.end(), EMPTY);
        }

        const Parallelepiped& get_parallelepiped() const
        {
                return m_parallelepiped;
        }

        void set_child(int child_number, int child_box_index)
        {
                ASSERT(child_number >= 0 && child_number < 8 && child_box_index >= 0);
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

template <typename Parallelepiped, typename OctreeObject>
void shrink_boxes(std::vector<OctreeBox<Parallelepiped, OctreeObject>>* boxes)
{
        boxes->shrink_to_fit();
        for (OctreeBox<Parallelepiped, OctreeObject>& box : *boxes)
        {
                box.shrink_objects();
        }
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
}

template <typename Parallelepiped, typename OctreeObject>
class Octree
{
        using OctreeBox = OctreeImplementation::OctreeBox<Parallelepiped, OctreeObject>;
        using Stack = std::stack<std::tuple<int, int>, std::vector<std::tuple<int, int>>>;

        // Смещение по лучу внутрь коробки от точки персечения с коробкой.
        static constexpr double DELTA = 10 * INTERSECTION_THRESHOLD;

        // Нижняя и верхняя границы для минимального количества объектов в коробке.
        static constexpr int MIN_OBJECTS_LEFT = 2;
        static constexpr int MIN_OBJECTS_RIGHT = 100;

        // Нижняя и верхняя границы для глубины дерева.
        static constexpr int MAX_DEPTH_LEFT = 1;
        static constexpr int MAX_DEPTH_RIGHT = 10;

        // Первым элементом массива является только 0.
        static constexpr int ROOT_BOX = 0;

        // Максимальная глубина и минимальное количество объектов для экземпляра дерева.
        const int MAX_DEPTH, MIN_OBJECTS;
        // Максимальное количество коробок — сумма геометрической прогресии со знаменателем 8.
        // Требуется для того, чтобы что-то отображать как расчёт.
        const int MAX_BOXES = (std::pow(8, MAX_DEPTH) - 1) / (8 - 1);

        // Все коробки хранятся в одном векторе
        std::vector<OctreeBox> m_boxes;

        int create_box(Parallelepiped&& box)
        {
                m_boxes.push_back(std::move(box));
                return m_boxes.size() - 1;
        }

        template <typename ShapeIntersection>
        void extend(Stack&& stack, const ShapeIntersection& functor_shape_intersection, ProgressRatio* progress)
        {
                while (!stack.empty())
                {
                        auto[depth, box] = stack.top();
                        stack.pop();

                        ASSERT(box >= ROOT_BOX && box < static_cast<int>(m_boxes.size()));

                        if (depth >= MAX_DEPTH || m_boxes[box].get_object_count() <= MIN_OBJECTS)
                        {
                                continue;
                        }

                        std::array<Parallelepiped, 8> child_parallelepipeds;

                        m_boxes[box].get_parallelepiped().binary_division(&child_parallelepipeds);

                        for (int child_number = 0; child_number < 8; ++child_number)
                        {
                                int child_box = create_box(std::move(child_parallelepipeds[child_number]));

                                m_boxes[box].set_child(child_number, child_box);

                                if (child_box & 0xfff)
                                {
                                        progress->set(child_box, MAX_BOXES);
                                }

                                for (const OctreeObject* obj : m_boxes[box].get_objects())
                                {
                                        if (functor_shape_intersection(&m_boxes[child_box].get_parallelepiped(), obj))
                                        {
                                                m_boxes[child_box].add_object(obj);
                                        }
                                }

                                stack.emplace(depth + 1, child_box);
                        }

                        m_boxes[box].delete_all_objects();
                }
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
               ProgressRatio* progress)
                : MAX_DEPTH(max_depth), MIN_OBJECTS(min_objects_per_box)
        {
                decompose(objects, functor_convex_hull_vertices, functor_shape_intersection, progress);
        }

        template <typename ConvexHullVertices, typename ShapeIntersection>
        void decompose(const std::vector<OctreeObject>& objects, const ConvexHullVertices& functor_convex_hull_vertices,
                       const ShapeIntersection& functor_shape_intersection, ProgressRatio* progress)

        {
                if (!(MAX_DEPTH >= MAX_DEPTH_LEFT && MAX_DEPTH <= MAX_DEPTH_RIGHT) ||
                    !(MIN_OBJECTS >= MIN_OBJECTS_LEFT && MIN_OBJECTS <= MIN_OBJECTS_RIGHT))
                {
                        error("Error limits for octree");
                }

                using namespace OctreeImplementation;

                m_boxes.clear();
                int root_box = create_box(cuboid_of_objects<Parallelepiped>(objects, functor_convex_hull_vertices));
                ASSERT(root_box == ROOT_BOX);
                m_boxes[root_box].set_all_objects(pointers_of_objects(objects));

                extend(Stack({{MAX_DEPTH_LEFT, ROOT_BOX}}), functor_shape_intersection, progress);

                shrink_boxes(&m_boxes);
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
