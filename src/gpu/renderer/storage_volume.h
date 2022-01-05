/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "storage.h"

#include "volume/object.h"

#include <src/com/log.h>

#include <functional>
#include <variant>

namespace ns::gpu::renderer
{
struct StorageVolumeCreate final
{
        std::unique_ptr<VolumeObject>* ptr;
        explicit StorageVolumeCreate(std::unique_ptr<VolumeObject>* const ptr) : ptr(ptr)
        {
        }
};

struct StorageVolumeVisibilityChanged final
{
};

struct StorageVolumeChanged final
{
        const VolumeObject::UpdateChanges* update_changes;
        explicit StorageVolumeChanged(const VolumeObject::UpdateChanges* const update_changes)
                : update_changes(update_changes)
        {
        }
};

using StorageVolumeEvents = std::variant<StorageVolumeChanged, StorageVolumeCreate, StorageVolumeVisibilityChanged>;

class StorageVolume final : private StorageEvents<VolumeObject>
{
        Storage<VolumeObject, VolumeObject> storage_;
        std::function<void(const StorageVolumeEvents&)> events_;

        void visibility_changed() override
        {
                events_(StorageVolumeVisibilityChanged());
        }

public:
        explicit StorageVolume(std::function<void(const StorageVolumeEvents&)>&& events)
                : storage_(this), events_(std::move(events))
        {
        }

        decltype(auto) visible_objects() const
        {
                return storage_.visible_objects();
        }

        decltype(auto) contains(const ObjectId id) const
        {
                return storage_.contains(id);
        }

        decltype(auto) erase(const ObjectId id)
        {
                return storage_.erase(id);
        }

        void clear()
        {
                storage_.clear();
        }

        void update(const volume::VolumeObject<3>& object)
        {
                VolumeObject* const ptr = [&]
                {
                        VolumeObject* const p = storage_.object(object.id());
                        if (p)
                        {
                                return p;
                        }
                        std::unique_ptr<VolumeObject> volume;
                        events_(StorageVolumeCreate(&volume));
                        return storage_.insert(object.id(), std::move(volume));
                }();

                VolumeObject::UpdateChanges update_changes;
                bool visible;

                try
                {
                        volume::Reading reading(object);
                        visible = reading.visible();
                        update_changes = ptr->update(reading);
                }
                catch (const std::exception& e)
                {
                        storage_.erase(object.id());
                        LOG(std::string("Error updating volume object. ") + e.what());
                        return;
                }
                catch (...)
                {
                        storage_.erase(object.id());
                        LOG("Unknown error updating volume object");
                        return;
                }

                const bool storage_visible = storage_.is_visible(object.id());

                if (visible && storage_visible)
                {
                        events_(StorageVolumeChanged(&update_changes));
                        return;
                }

                if (visible || storage_visible)
                {
                        storage_.set_visible(object.id(), visible);
                }
        }
};
}
