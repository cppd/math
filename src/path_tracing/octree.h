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

#include <algorithm>
#include <array>
#include <vector>

template <typename Parallelepiped, typename OctreeObject>
class Octree
{
        class OctreeBox
        {
                Parallelepiped m_parallelepiped;
                std::vector<const OctreeObject*> m_objects;
                std::array<OctreeBox*, 8> m_childs;

        public:
                OctreeBox(const Parallelepiped& box) : m_parallelepiped(box)
                {
                        std::fill(m_childs.begin(), m_childs.end(), nullptr);
                }

                const Parallelepiped& get_parallelepiped() const
                {
                        return m_parallelepiped;
                }

                void set_child(unsigned index, OctreeBox* box)
                {
                        ASSERT(index < 8);
                        m_childs[index] = box;
                }
                const std::array<OctreeBox*, 8>& get_childs() const
                {
                        return m_childs;
                }
                bool has_childs() const
                {
                        // Они или все заполнены, или все не заполнены
                        return m_childs[0] != nullptr;
                }

                void add_object(const OctreeObject* obj)
                {
                        m_objects.push_back(obj);
                }
                template <typename T>
                void add_objects(const T& obj)
                {
                        m_objects.insert(m_objects.cbegin(), obj.cbegin(), obj.cend());
                }
                void shrink_objects()
                {
                        m_objects.shrink_to_fit();
                }
                const std::vector<const OctreeObject*>& get_objects() const
                {
                        return m_objects;
                }
                unsigned get_object_count() const
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

        const unsigned MAX_DEPTH, MIN_OBJECTS;

        // Все коробки хранятся в одном векторе
        std::vector<OctreeBox> m_data;

        OctreeBox* create_box(const Parallelepiped& box)
        {
                m_data.push_back(box);
                return &m_data[m_data.size() - 1];
        }

        const OctreeBox* root() const
        {
                return &m_data[0];
        }

        void extend(unsigned depth, OctreeBox* box)
        {
                if (depth >= MAX_DEPTH || box->get_object_count() <= MIN_OBJECTS)
                {
                        return;
                }

                vec3 half0 = box->get_parallelepiped().e0() / 2.0;
                vec3 half1 = box->get_parallelepiped().e1() / 2.0;
                vec3 half2 = box->get_parallelepiped().e2() / 2.0;

                std::array<vec3, 8> orgs;

                orgs[0] = box->get_parallelepiped().org();
                orgs[1] = orgs[0] + half0;
                orgs[2] = orgs[0] + half1;
                orgs[3] = orgs[2] + half0;
                orgs[4] = box->get_parallelepiped().org() + half2;
                orgs[5] = orgs[4] + half0;
                orgs[6] = orgs[4] + half1;
                orgs[7] = orgs[6] + half0;

                for (unsigned index = 0; index < 8; ++index)
                {
                        OctreeBox* child_box = create_box(Parallelepiped(orgs[index], half0, half1, half2));

                        box->set_child(index, child_box);

                        for (const OctreeObject* obj : box->get_objects())
                        {
                                if (obj->intersect_shape(child_box->get_parallelepiped()))
                                {
                                        child_box->add_object(obj);
                                }
                        }
                }

                box->delete_all_objects();

                for (OctreeBox* b : box->get_childs())
                {
                        extend(depth + 1, b);
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

                for (const OctreeBox* child_box : box->get_childs())
                {
                        find_point_box_impl(child_box, p, found_box);
                        if (found_box)
                        {
                                return;
                        }
                }
        }

        void find_point_box(const vec3& p, const OctreeBox** box) const
        {
                *box = nullptr;
                find_point_box_impl(root(), p, box);
        }

        Parallelepiped root_parallelepiped(const std::vector<const OctreeObject*>& objects)
        {
                constexpr double MAX = std::numeric_limits<double>::max();
                constexpr double MIN = std::numeric_limits<double>::lowest();

                vec3 min(MAX, MAX, MAX);
                vec3 max(MIN, MIN, MIN);

                std::vector<vec3> vertices;

                for (const OctreeObject* o : objects)
                {
                        o->convex_hull_vertices(&vertices);
                        for (const vec3& v : vertices)
                        {
                                for (int i = 0; i < 3; ++i)
                                {
                                        min[i] = std::min(v[i], min[i]);
                                        max[i] = std::max(v[i], max[i]);
                                }
                        }
                }

                vec3 d = max - min;

                return Parallelepiped(min, vec3(d[0], 0, 0), vec3(0, d[1], 0), vec3(0, 0, d[2]));
        }

public:
        // Используются указатели на элементы массива вместо индексов элементов в массиве
        Octree(const Octree&) = delete;
        Octree& operator=(const Octree&) = delete;

        Octree(int max_depth, int max_objects_per_box, const std::vector<const OctreeObject*>& objects)
                : MAX_DEPTH(max_depth), MIN_OBJECTS(max_objects_per_box)
        {
                OctreeBox* box = create_box(root_parallelepiped(objects));
                box->add_objects(objects);
                extend(1, box);
        }

        template <typename T>
        bool trace_ray(ray3 ray, T find_intersection) const
        {
                bool first = true;

                while (true)
                {
                        const OctreeBox* box;

                        find_point_box(ray.get_org(), &box);

                        if (box)
                        {
                                if (find_intersection(box->get_objects()))
                                {
                                        return true;
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
                                        // пересечение с самим октадеревом.
                                        box = root();
                                }
                        }

                        double t;
                        if (!box->get_parallelepiped().intersect(ray, &t))
                        {
                                return false;
                        }

                        ray.set_org(ray.point(t + DELTA));

                        first = false;
                }
        }
};
