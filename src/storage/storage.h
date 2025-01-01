/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/model/object_id.h>
#include <src/model/volume_object.h>

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace ns::storage
{
class Storage final
{
        mutable std::shared_mutex mutex_;
        std::unordered_map<model::ObjectId, std::variant<MeshObject, VolumeObject>> map_;

        template <typename To, typename From>
        [[nodiscard]] static To to_type(From&& object)
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
        void delete_object(const model::ObjectId id)
        {
                decltype(map_)::mapped_type tmp;
                const std::unique_lock lock(mutex_);
                const auto iter = map_.find(id);
                if (iter != map_.cend())
                {
                        tmp = std::move(iter->second);
                        map_.erase(iter);
                }
        }

        void clear()
        {
                decltype(map_) tmp;
                const std::unique_lock lock(mutex_);
                tmp = std::move(map_);
                map_.clear();
        }

        template <std::size_t N>
        void set_object(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
        {
                if (!object)
                {
                        error("No mesh object to set in storage");
                }

                const std::unique_lock lock(mutex_);
                const auto iter = map_.find(object->id());
                if (iter == map_.cend())
                {
                        map_.emplace(object->id(), object);
                        return;
                }

                ASSERT(std::holds_alternative<MeshObject>(iter->second));
                ASSERT(std::holds_alternative<std::shared_ptr<model::mesh::MeshObject<N>>>(
                        std::get<MeshObject>(iter->second)));
                ASSERT(object
                       == std::get<std::shared_ptr<model::mesh::MeshObject<N>>>(std::get<MeshObject>(iter->second)));
        }

        template <std::size_t N>
        void set_object(const std::shared_ptr<model::volume::VolumeObject<N>>& object)
        {
                if (!object)
                {
                        error("No mesh object to set in storage");
                }

                const std::unique_lock lock(mutex_);
                const auto iter = map_.find(object->id());
                if (iter == map_.cend())
                {
                        map_.emplace(object->id(), object);
                        return;
                }

                ASSERT(std::holds_alternative<VolumeObject>(iter->second));
                ASSERT(std::holds_alternative<std::shared_ptr<model::volume::VolumeObject<N>>>(
                        std::get<VolumeObject>(iter->second)));
                ASSERT(object
                       == std::get<std::shared_ptr<model::volume::VolumeObject<N>>>(
                               std::get<VolumeObject>(iter->second)));
        }

        [[nodiscard]] std::optional<MeshObject> mesh_object(const model::ObjectId id) const
        {
                const std::shared_lock lock(mutex_);
                const auto iter = map_.find(id);
                if (iter != map_.cend())
                {
                        if (const MeshObject* const p = std::get_if<MeshObject>(&iter->second))
                        {
                                return *p;
                        }
                }
                return std::nullopt;
        }

        [[nodiscard]] std::optional<MeshObjectConst> mesh_object_const(const model::ObjectId id) const
        {
                if (auto object = mesh_object(id))
                {
                        return to_type<MeshObjectConst>(std::move(*object));
                }
                return std::nullopt;
        }

        [[nodiscard]] std::optional<VolumeObject> volume_object(const model::ObjectId id) const
        {
                const std::shared_lock lock(mutex_);
                const auto iter = map_.find(id);
                if (iter != map_.cend())
                {
                        if (const VolumeObject* const p = std::get_if<VolumeObject>(&iter->second))
                        {
                                return *p;
                        }
                }
                return std::nullopt;
        }

        [[nodiscard]] std::optional<VolumeObjectConst> volume_object_const(const model::ObjectId id) const
        {
                if (auto object = volume_object(id))
                {
                        return to_type<VolumeObjectConst>(std::move(*object));
                }
                return std::nullopt;
        }

        template <typename T>
        [[nodiscard]] std::vector<T> objects() const
        {
                static_assert(std::is_same_v<T, MeshObjectConst> || std::is_same_v<T, VolumeObjectConst>);

                using OBJ = std::conditional_t<std::is_same_v<T, MeshObjectConst>, MeshObject, VolumeObject>;

                std::vector<T> objects;
                const std::shared_lock lock(mutex_);
                for (const auto& v : map_)
                {
                        if (std::holds_alternative<OBJ>(v.second))
                        {
                                objects.push_back(to_type<T>(std::get<OBJ>(v.second)));
                        }
                }
                return objects;
        }
};
}
