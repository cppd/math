/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "obj/obj.h"

#include <memory>
#include <unordered_map>

// GLSL имеет размер float == 4
constexpr int GLSL_STD430_VEC3_ALIGN = 4 * 4; // для vec3 выравнивание по 4 * N
static_assert(sizeof(vec2f) == 2 * sizeof(float));
static_assert(sizeof(vec3f) == 3 * sizeof(float));

enum class DrawType
{
        Points,
        Lines,
        Triangles
};

DrawType draw_type_of_obj(const Obj<3>* obj);

template <typename T>
class DrawObjects final
{
        struct MapEntry
        {
                std::unique_ptr<T> object;
                int scale_object_id;
                MapEntry(std::unique_ptr<T>&& obj_, int scale_id_) : object(std::move(obj_)), scale_object_id(scale_id_)
                {
                }
        };

        std::unordered_map<int, MapEntry> m_objects;

        const T* m_draw_object = nullptr;
        const T* m_draw_scale_object = nullptr;
        int m_draw_scale_object_id = 0;

public:
        void add_object(std::unique_ptr<T>&& object, int id, int scale_id)
        {
                if (m_draw_object && id == m_draw_scale_object_id)
                {
                        m_draw_scale_object = object.get();
                }

                m_objects.insert_or_assign(id, MapEntry(std::move(object), scale_id));
        }

        void delete_object(int id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        if (iter->second.object.get() == m_draw_object)
                        {
                                m_draw_object = nullptr;
                        }
                        if (iter->second.object.get() == m_draw_scale_object)
                        {
                                m_draw_scale_object = nullptr;
                        }
                        m_objects.erase(iter);
                }
        }

        void show_object(int id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        m_draw_object = iter->second.object.get();

                        m_draw_scale_object_id = iter->second.scale_object_id;

                        auto scale_iter = m_objects.find(m_draw_scale_object_id);
                        if (scale_iter != m_objects.cend())
                        {
                                m_draw_scale_object = scale_iter->second.object.get();
                        }
                        else
                        {
                                m_draw_scale_object = nullptr;
                        }
                }
                else
                {
                        m_draw_object = nullptr;
                        m_draw_scale_object = nullptr;
                }
        }

        void delete_all()
        {
                m_objects.clear();
                m_draw_object = nullptr;
                m_draw_scale_object = nullptr;
        }

        const T* object() const
        {
                return m_draw_object;
        }

        const T* scale_object() const
        {
                return m_draw_scale_object ? m_draw_scale_object : m_draw_object;
        }
};
