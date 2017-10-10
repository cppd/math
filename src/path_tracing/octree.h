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
#include "ray3.h"
#include "vec3.h"

#include "progress/progress.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

template <typename Parallelepiped, typename OctreeObject>
class Octree
{
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

        static constexpr double DELTA = 10 * INTERSECTION_THRESHOLD;

        static constexpr int LIMIT_MAX_DEPTH = 10;

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
        void extend(int depth, int parent_box, const ShapeIntersection& functor_shape_intersection, ProgressRatio* progress)
        {
                ASSERT(parent_box >= 0 && parent_box < static_cast<int>(m_boxes.size()));

                if (depth >= MAX_DEPTH || m_boxes[parent_box].get_object_count() <= MIN_OBJECTS)
                {
                        return;
                }

                std::array<Parallelepiped, 8> child_parallelepipeds;

                m_boxes[parent_box].get_parallelepiped().binary_division(&child_parallelepipeds);

                for (int child_number = 0; child_number < 8; ++child_number)
                {
                        int child_box = create_box(std::move(child_parallelepipeds[child_number]));

                        m_boxes[parent_box].set_child(child_number, child_box);

                        if (child_box & 0xfff)
                        {
                                progress->set(child_box, MAX_BOXES);
                        }

                        for (const OctreeObject* obj : m_boxes[parent_box].get_objects())
                        {
                                if (functor_shape_intersection(&m_boxes[child_box].get_parallelepiped(), obj))
                                {
                                        m_boxes[child_box].add_object(obj);
                                }
                        }
                }

                m_boxes[parent_box].delete_all_objects();

                for (int child_number = 0; child_number < 8; ++child_number)
                {
                        int child_box = m_boxes[parent_box].get_childs()[child_number];
                        extend(depth + 1, child_box, functor_shape_intersection, progress);
                }
        }

        void find_point_box_impl(const OctreeBox* box, const vec3& p, const OctreeBox** found_box) const
        {
                if (!box->get_parallelepiped().inside(p))
                {
                        return;
                }

                if (!box->has_childs())
                {
                        *found_box = box;
                        return;
                }

                for (int child_box : box->get_childs())
                {
                        find_point_box_impl(&m_boxes[child_box], p, found_box);
                        if (*found_box)
                        {
                                return;
                        }
                }
        }

        void find_point_box(const vec3& p, const OctreeBox** box) const
        {
                *box = nullptr;
                find_point_box_impl(&m_boxes[0], p, box);
        }

        template <typename ConvexHullVertices>
        Parallelepiped root_parallelepiped(const std::vector<const OctreeObject*>& objects,
                                           const ConvexHullVertices& functor_convex_hull_vertices)
        {
                // Прямоугольный параллелепипед, параллельный координатным плоскостям

                constexpr double MAX = std::numeric_limits<double>::max();
                constexpr double MIN = std::numeric_limits<double>::lowest();

                vec3 min(MAX, MAX, MAX);
                vec3 max(MIN, MIN, MIN);

                for (const OctreeObject* object : objects)
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
                ASSERT(MAX_DEPTH >= 1 && MAX_DEPTH <= LIMIT_MAX_DEPTH);
                ASSERT(MIN_OBJECTS >= 2 && MIN_OBJECTS <= 100);

                m_boxes.clear();

                std::vector<const OctreeObject*> object_pointers;
                object_pointers.reserve(objects.size());
                for (const OctreeObject& object : objects)
                {
                        object_pointers.push_back(&object);
                }

                int root_box = create_box(root_parallelepiped(object_pointers, functor_convex_hull_vertices));

                ASSERT(root_box == 0);

                m_boxes[root_box].set_all_objects(std::move(object_pointers));

                extend(1 /*номера уровней начинаются с 1*/, root_box, functor_shape_intersection, progress);

                m_boxes.shrink_to_fit();
        }

        bool intersect_root(const ray3& ray, double* t) const
        {
                return m_boxes[0].get_parallelepiped().intersect(ray, t);
        }

        // Вызывается после intersect_root. Если в intersect_root пересечение было найдено,
        // то сюда передаётся результат пересечения в параметре root_t.
        template <typename FindIntersection>
        bool trace_ray(ray3 ray, double root_t, const FindIntersection& functor_find_intersection) const
        {
                bool first = true;

                while (true)
                {
                        const OctreeBox* box;

                        find_point_box(ray.get_org(), &box);

                        double t;

                        if (box)
                        {
                                if (box->get_objects().size() > 0 && functor_find_intersection(box->get_objects()))
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
