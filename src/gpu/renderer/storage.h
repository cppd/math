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
#include <unordered_map>
#include <unordered_set>

namespace ns::gpu::renderer
{
template <typename T>
class ObjectStorage final
{
        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);

        using VisibleType = std::conditional_t<std::is_same_v<T, VolumeObject>, T, const T>;

        std::unordered_map<ObjectId, std::unique_ptr<T>> map_;
        std::unordered_set<VisibleType*> visible_objects_;
        std::function<void()> visibility_changed_;

public:
        explicit ObjectStorage(std::function<void()>&& visibility_changed)
                : visibility_changed_(std::move(visibility_changed))
        {
                ASSERT(visibility_changed_);
        }

        T* insert(ObjectId id, std::unique_ptr<T>&& object)
        {
                const auto pair = map_.emplace(id, std::move(object));
                ASSERT(pair.second);
                return pair.first->second.get();
        }

        bool erase(ObjectId id)
        {
                auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                bool visibility_changed = visible_objects_.erase(iter->second.get()) > 0;
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
                bool visibility_changed = !visible_objects_.empty();
                visible_objects_.clear();
                map_.clear();
                if (visibility_changed)
                {
                        visibility_changed_();
                }
        }

        T* find(ObjectId id) const
        {
                auto iter = map_.find(id);
                return (iter != map_.cend()) ? iter->second.get() : nullptr;
        }

        bool set_visible(ObjectId id, bool visible)
        {
                auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }

                VisibleType* ptr = iter->second.get();
                auto iter_v = visible_objects_.find(ptr);
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

        bool is_visible(ObjectId id) const
        {
                auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                return visible_objects_.contains(iter->second.get());
        }
};

}
