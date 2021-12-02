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

namespace ns::gpu::renderer
{

template <typename T>
class ObjectStorageEvents
{
protected:
        ~ObjectStorageEvents() = default;

public:
        virtual std::unique_ptr<T> create_object() const = 0;
        virtual void visibility_changed() = 0;
};

template <typename T>
class ObjectStorage final
{
        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);

        using VisibleType = std::conditional_t<std::is_same_v<T, VolumeObject>, T, const T>;

        static constexpr std::size_t EMPTY = -1;
        struct Object final
        {
                std::unique_ptr<T> ptr;
                std::size_t visible_index = EMPTY;
                explicit Object(std::unique_ptr<T>&& ptr) : ptr(std::move(ptr))
                {
                }
        };

        ObjectStorageEvents<T>* events_;

        std::unordered_map<ObjectId, Object> map_;
        std::vector<VisibleType*> visible_;
        std::vector<Object*> visible_ptr_;

        void erase_visible(const std::size_t index)
        {
                ASSERT(index < visible_.size());
                ASSERT(visible_.size() == visible_ptr_.size());
                visible_[index] = visible_.back();
                visible_.pop_back();
                visible_ptr_[index] = visible_ptr_.back();
                visible_ptr_[index]->visible_index = index;
                visible_ptr_.pop_back();
        }

public:
        explicit ObjectStorage(ObjectStorageEvents<T>* const events) : events_(events)
        {
        }

        bool erase(const ObjectId id)
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }

                const auto visible_index = iter->second.visible_index;
                const bool visibility_changed = visible_index != EMPTY;
                if (visibility_changed)
                {
                        erase_visible(visible_index);
                }
                map_.erase(iter);
                if (visibility_changed)
                {
                        events_->visibility_changed();
                }
                return true;
        }

        bool empty() const
        {
                ASSERT(!map_.empty() || visible_.empty());
                return map_.empty();
        }

        void clear()
        {
                const bool visibility_changed = !visible_.empty();
                visible_.clear();
                map_.clear();
                if (visibility_changed)
                {
                        events_->visibility_changed();
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
                        return iter->second.ptr.get();
                }
                const auto pair = map_.emplace(id, events_->create_object());
                ASSERT(pair.second);
                return pair.first->second.ptr.get();
        }

        bool set_visible(const ObjectId id, const bool visible)
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }

                std::size_t& visible_index = iter->second.visible_index;
                if (!visible)
                {
                        if (visible_index != EMPTY)
                        {
                                erase_visible(visible_index);
                                visible_index = EMPTY;
                                events_->visibility_changed();
                        }
                }
                else if (visible_index == EMPTY)
                {
                        ASSERT(visible_.size() == visible_ptr_.size());
                        visible_index = visible_.size();
                        visible_.push_back(iter->second.ptr.get());
                        visible_ptr_.push_back(&iter->second);
                        events_->visibility_changed();
                }
                return true;
        }

        const std::vector<VisibleType*>& visible_objects() const
        {
                return visible_;
        }

        bool is_visible(const ObjectId id) const
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                return iter->second.visible_index != EMPTY;
        }
};

class ObjectStorageEventsMesh : public ObjectStorageEvents<MeshObject>
{
        virtual std::unique_ptr<MeshObject> create_mesh() const = 0;
        virtual void mesh_visibility_changed() = 0;

        virtual std::unique_ptr<MeshObject> create_object() const final
        {
                return create_mesh();
        }

        virtual void visibility_changed() final
        {
                mesh_visibility_changed();
        }

protected:
        ~ObjectStorageEventsMesh() = default;
};

class ObjectStorageEventsVolume : public ObjectStorageEvents<VolumeObject>
{
        virtual std::unique_ptr<VolumeObject> create_volume() const = 0;
        virtual void volume_visibility_changed() = 0;

        virtual std::unique_ptr<VolumeObject> create_object() const final
        {
                return create_volume();
        }

        virtual void visibility_changed() final
        {
                volume_visibility_changed();
        }

protected:
        ~ObjectStorageEventsVolume() = default;
};
}
