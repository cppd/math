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

#include "mesh_object.h"
#include "volume_object.h"

#include <src/com/error.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace gpu::renderer
{
template <typename T>
class ObjectStorage final
{
        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);

        using VisibleType = std::conditional_t<std::is_same_v<T, VolumeObject>, T, const T>;

        std::unordered_map<ObjectId, std::unique_ptr<T>> m_map;
        std::unordered_set<VisibleType*> m_visible_objects;
        std::function<void()> m_visibility_changed;

public:
        explicit ObjectStorage(std::function<void()>&& visibility_changed)
                : m_visibility_changed(std::move(visibility_changed))
        {
                ASSERT(m_visibility_changed);
        }

        T* insert(ObjectId id, std::unique_ptr<T>&& object)
        {
                const auto pair = m_map.emplace(id, std::move(object));
                ASSERT(pair.second);
                return pair.first->second.get();
        }

        bool erase(ObjectId id)
        {
                auto iter = m_map.find(id);
                if (iter == m_map.cend())
                {
                        return false;
                }
                bool visibility_changed = m_visible_objects.erase(iter->second.get()) > 0;
                m_map.erase(iter);
                if (visibility_changed)
                {
                        m_visibility_changed();
                }
                return true;
        }

        bool empty() const
        {
                ASSERT(!m_map.empty() || m_visible_objects.empty());
                return m_map.empty();
        }

        void clear()
        {
                bool visibility_changed = !m_visible_objects.empty();
                m_visible_objects.clear();
                m_map.clear();
                if (visibility_changed)
                {
                        m_visibility_changed();
                }
        }

        T* find(ObjectId id) const
        {
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.get() : nullptr;
        }

        bool set_visible(ObjectId id, bool visible)
        {
                auto iter = m_map.find(id);
                if (iter == m_map.cend())
                {
                        return false;
                }

                VisibleType* ptr = iter->second.get();
                auto iter_v = m_visible_objects.find(ptr);
                if (!visible)
                {
                        if (iter_v != m_visible_objects.cend())
                        {
                                m_visible_objects.erase(iter_v);
                                m_visibility_changed();
                        }
                }
                else if (iter_v == m_visible_objects.cend())
                {
                        m_visible_objects.insert(ptr);
                        m_visibility_changed();
                }
                return true;
        }

        const std::unordered_set<VisibleType*>& visible_objects() const
        {
                return m_visible_objects;
        }

        bool is_visible(ObjectId id) const
        {
                auto iter = m_map.find(id);
                if (iter == m_map.cend())
                {
                        return false;
                }
                return m_visible_objects.contains(iter->second.get());
        }
};

}
