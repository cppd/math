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

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

namespace gpu
{
template <typename Id, typename T>
class RendererObjectStorage final
{
        struct MapEntry
        {
                std::unique_ptr<T> object;
                MapEntry(std::unique_ptr<T>&& object) : object(std::move(object))
                {
                }
        };

        std::unordered_map<Id, MapEntry> m_objects;

        const T* m_object = nullptr;
        std::optional<Id> m_object_id;

public:
        void add_object(std::unique_ptr<T>&& object, Id id)
        {
                m_objects.insert_or_assign(id, MapEntry(std::move(object)));
        }

        bool is_current_object(Id id) const
        {
                return m_object && m_object_id && id == *m_object_id;
        }

        void delete_object(Id id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        if (iter->second.object.get() == m_object)
                        {
                                m_object = nullptr;
                        }
                        m_objects.erase(iter);
                }
        }

        void show_object(Id id)
        {
                auto iter = m_objects.find(id);
                if (iter != m_objects.cend())
                {
                        m_object = iter->second.object.get();
                        m_object_id = id;
                }
                else
                {
                        m_object = nullptr;
                        m_object_id.reset();
                }
        }

        void delete_all()
        {
                m_objects.clear();
                m_object = nullptr;
                m_object_id.reset();
        }

        const T* object() const
        {
                return m_object;
        }
};
}
