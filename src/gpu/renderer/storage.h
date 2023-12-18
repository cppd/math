/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/error.h>
#include <src/model/object_id.h>

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ns::gpu::renderer
{
template <typename T>
class StorageEvents
{
protected:
        ~StorageEvents() = default;

public:
        virtual void visibility_changed() = 0;
};

template <typename T, typename VisibleType>
class Storage final
{
        static constexpr std::size_t EMPTY = -1;

        struct Object final
        {
                std::unique_ptr<T> ptr;
                std::size_t visible_index = EMPTY;

                explicit Object(std::unique_ptr<T>&& ptr)
                        : ptr(std::move(ptr))
                {
                }
        };

        StorageEvents<T>* events_;

        std::unordered_map<model::ObjectId, Object> map_;
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
        explicit Storage(StorageEvents<T>* const events)
                : events_(events)
        {
        }

        bool erase(const model::ObjectId id)
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

        [[nodiscard]] bool empty() const
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

        [[nodiscard]] bool contains(const model::ObjectId id) const
        {
                return map_.contains(id);
        }

        [[nodiscard]] T* object(const model::ObjectId id)
        {
                const auto iter = map_.find(id);
                if (iter != map_.cend())
                {
                        return iter->second.ptr.get();
                }
                return nullptr;
        }

        [[nodiscard]] T* insert(const model::ObjectId id, std::unique_ptr<T>&& object)
        {
                const auto pair = map_.emplace(id, std::move(object));
                ASSERT(pair.second);
                return pair.first->second.ptr.get();
        }

        void set_visible(const model::ObjectId id, const bool visible)
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return;
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
        }

        [[nodiscard]] const std::vector<VisibleType*>& visible_objects() const
        {
                return visible_;
        }

        [[nodiscard]] bool is_visible(const model::ObjectId id) const
        {
                const auto iter = map_.find(id);
                if (iter == map_.cend())
                {
                        return false;
                }
                return iter->second.visible_index != EMPTY;
        }
};
}
