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

#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace storage
{
template <size_t N>
class Storage
{
        static_assert(N >= 3);

        using MeshData = std::shared_ptr<mesh::MeshObject<N>>;
        using VolumeData = std::shared_ptr<volume::VolumeObject<N>>;

        mutable std::shared_mutex m_mutex;
        std::unordered_map<ObjectId, std::variant<MeshData, VolumeData>> m_map;

public:
        template <typename T>
        void set_mesh_object(T&& object)
        {
                std::unique_lock lock(m_mutex);
                auto iter = m_map.find(object->id());
                if (iter == m_map.cend())
                {
                        m_map.emplace(object->id(), std::forward<T>(object));
                        return;
                }
                ASSERT(std::holds_alternative<MeshData>(iter->second));
                ASSERT(std::get<MeshData>(iter->second) == object);
        }

        template <typename T>
        void set_volume_object(T&& object)
        {
                std::unique_lock lock(m_mutex);
                auto iter = m_map.find(object->id());
                if (iter == m_map.cend())
                {
                        m_map.emplace(object->id(), std::forward<T>(object));
                        return;
                }
                ASSERT(std::holds_alternative<VolumeData>(iter->second));
                ASSERT(std::get<VolumeData>(iter->second) == object);
        }

        //

        std::shared_ptr<mesh::MeshObject<N>> mesh_object(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                if (iter != m_map.cend())
                {
                        try
                        {
                                return std::get<MeshData>(iter->second);
                        }
                        catch (const std::bad_variant_access&)
                        {
                        }
                }
                return nullptr;
        }

        std::shared_ptr<volume::VolumeObject<N>> volume_object(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                if (iter != m_map.cend())
                {
                        try
                        {
                                return std::get<VolumeData>(iter->second);
                        }
                        catch (const std::bad_variant_access&)
                        {
                        }
                }
                return nullptr;
        }

        //

        void delete_object(ObjectId id)
        {
                typename decltype(m_map)::mapped_type tmp;
                std::unique_lock lock(m_mutex);
                auto iter = m_map.find(id);
                if (iter != m_map.cend())
                {
                        tmp = std::move(iter->second);
                        m_map.erase(iter);
                }
        }

        void clear()
        {
                decltype(m_map) tmp;
                std::unique_lock lock(m_mutex);
                tmp = std::move(m_map);
                m_map.clear();
        }
};
}
