/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "types.h"

#include <src/com/error.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <variant>

namespace ns::storage
{
class Storage final
{
        mutable std::shared_mutex m_mutex;
        std::unordered_map<ObjectId, std::variant<MeshObject, VolumeObject>> m_map;

        template <typename To, typename From>
        static To to_type(From&& object)
        {
                static_assert(!std::is_same_v<std::remove_cvref_t<From>, std::remove_cvref_t<To>>);

                return std::visit(
                        [](auto&& v)
                        {
                                return To(std::forward<decltype(v)>(v));
                        },
                        std::forward<From>(object));
        }

public:
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

        template <std::size_t N>
        void set_mesh_object(const std::shared_ptr<mesh::MeshObject<N>>& object)
        {
                if (!object)
                {
                        error("No mesh object to set in storage");
                }

                std::unique_lock lock(m_mutex);
                auto iter = m_map.find(object->id());
                if (iter == m_map.cend())
                {
                        m_map.emplace(object->id(), object);
                        return;
                }

                ASSERT(std::holds_alternative<MeshObject>(iter->second));
                ASSERT(std::holds_alternative<std::shared_ptr<mesh::MeshObject<N>>>(
                        std::get<MeshObject>(iter->second)));
                ASSERT(object == std::get<std::shared_ptr<mesh::MeshObject<N>>>(std::get<MeshObject>(iter->second)));
        }

        template <std::size_t N>
        void set_volume_object(const std::shared_ptr<volume::VolumeObject<N>>& object)
        {
                if (!object)
                {
                        error("No mesh object to set in storage");
                }

                std::unique_lock lock(m_mutex);
                auto iter = m_map.find(object->id());
                if (iter == m_map.cend())
                {
                        m_map.emplace(object->id(), object);
                        return;
                }

                ASSERT(std::holds_alternative<VolumeObject>(iter->second));
                ASSERT(std::holds_alternative<std::shared_ptr<volume::VolumeObject<N>>>(
                        std::get<VolumeObject>(iter->second)));
                ASSERT(object
                       == std::get<std::shared_ptr<volume::VolumeObject<N>>>(std::get<VolumeObject>(iter->second)));
        }

        std::optional<MeshObject> mesh_object(ObjectId id) const
        {
                std::optional<MeshObject> opt;

                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                if (iter != m_map.cend())
                {
                        try
                        {
                                opt = std::get<MeshObject>(iter->second);
                        }
                        catch (const std::bad_variant_access&)
                        {
                        }
                }

                return opt;
        }

        std::optional<MeshObjectConst> mesh_object_const(ObjectId id) const
        {
                std::optional<MeshObject> opt = mesh_object(id);
                if (opt)
                {
                        return to_type<MeshObjectConst>(*opt);
                }
                return std::nullopt;
        }

        std::optional<VolumeObject> volume_object(ObjectId id) const
        {
                std::optional<VolumeObject> opt;

                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                if (iter != m_map.cend())
                {
                        try
                        {
                                opt = std::get<VolumeObject>(iter->second);
                        }
                        catch (const std::bad_variant_access&)
                        {
                        }
                }

                return opt;
        }

        std::optional<VolumeObjectConst> volume_object_const(ObjectId id) const
        {
                std::optional<VolumeObject> opt = volume_object(id);
                if (opt)
                {
                        return to_type<VolumeObjectConst>(*opt);
                }
                return std::nullopt;
        }

        template <typename T>
        std::vector<T> objects() const
        {
                std::vector<T> objects;
                std::shared_lock lock(m_mutex);
                if constexpr (std::is_same_v<T, MeshObjectConst> || std::is_same_v<T, VolumeObjectConst>)
                {
                        using OBJ = std::conditional_t<std::is_same_v<T, MeshObjectConst>, MeshObject, VolumeObject>;
                        for (const auto& v : m_map)
                        {
                                if (std::holds_alternative<OBJ>(v.second))
                                {
                                        objects.push_back(to_type<T>(std::get<OBJ>(v.second)));
                                }
                        }
                }
                else
                {
                        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);
                        for (const auto& v : m_map)
                        {
                                if (std::holds_alternative<T>(v.second))
                                {
                                        objects.push_back(std::get<T>(v.second));
                                }
                        }
                }
                return objects;
        }
};
}
