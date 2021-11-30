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

#include "mesh_object.h"
#include "volume_object.h"

#include <src/com/error.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ns::gpu::renderer
{
template <typename T>
class ObjectStorage final
{
        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);

        using VisibleType = std::conditional_t<std::is_same_v<T, VolumeObject>, T, const T>;

        std::function<std::unique_ptr<T>()> create_object_;
        std::function<void()> visibility_changed_;

        std::unordered_map<ObjectId, std::unique_ptr<T>> map_;
        std::unordered_set<VisibleType*> visible_objects_;

public:
        explicit ObjectStorage(
                std::function<std::unique_ptr<T>()>&& create_object,
                std::function<void()>&& visibility_changed)
                : create_object_(std::move(create_object)), visibility_changed_(std::move(visibility_changed))
        {
                ASSERT(create_object_);
                ASSERT(visibility_changed_);
        }

        bool erase(const ObjectId id)
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                const bool visibility_changed = visible_objects_.erase(iter->second.get()) > 0;
                map_.erase(iter);
                if (visibility_changed)
                {
                        visibility_changed_();
                }
                return true;
        }

        bool empty() const
        {
                ASSERT(!map_.empty() || visible_objects_.empty());
                return map_.empty();
        }

        void clear()
        {
                const bool visibility_changed = !visible_objects_.empty();
                visible_objects_.clear();
                map_.clear();
                if (visibility_changed)
                {
                        visibility_changed_();
                }
        }

        bool contains(const ObjectId id) const
        {
                return map_.contains(id);
        }

        T* object(const ObjectId id)
        {
                const auto iter = map_.find(id);
                if (iter != map_.cend())
                {
                        return iter->second.get();
                }
                const auto pair = map_.emplace(id, create_object_());
                ASSERT(pair.second);
                return pair.first->second.get();
        }

        bool set_visible(const ObjectId id, const bool visible)
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }

                VisibleType* const ptr = iter->second.get();
                const auto iter_v = visible_objects_.find(ptr);
                if (!visible)
                {
                        if (iter_v != visible_objects_.cend())
                        {
                                visible_objects_.erase(iter_v);
                                visibility_changed_();
                        }
                }
                else if (iter_v == visible_objects_.cend())
                {
                        visible_objects_.insert(ptr);
                        visibility_changed_();
                }
                return true;
        }

        const std::unordered_set<VisibleType*>& visible_objects() const
        {
                return visible_objects_;
        }

        bool is_visible(const ObjectId id) const
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                return visible_objects_.contains(iter->second.get());
        }
};

}
