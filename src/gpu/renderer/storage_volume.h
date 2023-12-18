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

#include "storage.h"

#include "volume/object.h"

#include <src/com/log.h>

#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace ns::gpu::renderer
{
class StorageVolumeEvents
{
protected:
        ~StorageVolumeEvents() = default;

public:
        virtual std::unique_ptr<VolumeObject> volume_create() = 0;
        virtual void volume_visibility_changed() = 0;
        virtual void volume_visible_changed(const VolumeObject::UpdateChanges& update_changes) = 0;
};

class StorageVolume final : private StorageEvents<VolumeObject>
{
        Storage<VolumeObject, VolumeObject> storage_;
        StorageVolumeEvents* events_;

        void visibility_changed() override
        {
                events_->volume_visibility_changed();
        }

        struct Updates final
        {
                bool visible;
                VolumeObject::UpdateChanges changes;
        };

        std::optional<Updates> update_volume(const model::volume::VolumeObject<3>& object)
        {
                VolumeObject* const ptr = [&]
                {
                        if (VolumeObject* const p = storage_.object(object.id()))
                        {
                                return p;
                        }
                        return storage_.insert(object.id(), events_->volume_create());
                }();

                try
                {
                        const model::volume::Reading reading(object);
                        return Updates{.visible = reading.visible(), .changes = ptr->update(reading)};
                }
                catch (const std::exception& e)
                {
                        storage_.erase(object.id());
                        LOG(std::string("Error updating volume object: ") + e.what());
                        return std::nullopt;
                }
                catch (...)
                {
                        storage_.erase(object.id());
                        LOG("Unknown error updating volume object");
                        return std::nullopt;
                }
        }

public:
        explicit StorageVolume(StorageVolumeEvents* const events)
                : storage_(this),
                  events_(events)
        {
        }

        [[nodiscard]] decltype(auto) visible_objects() const
        {
                return storage_.visible_objects();
        }

        [[nodiscard]] decltype(auto) contains(const model::ObjectId id) const
        {
                return storage_.contains(id);
        }

        decltype(auto) erase(const model::ObjectId id)
        {
                return storage_.erase(id);
        }

        void clear()
        {
                storage_.clear();
        }

        void update(const model::volume::VolumeObject<3>& object)
        {
                const std::optional<Updates> updates = update_volume(object);

                if (!updates)
                {
                        return;
                }

                const bool storage_visible = storage_.is_visible(object.id());

                if (updates->visible && storage_visible)
                {
                        events_->volume_visible_changed(updates->changes);
                        return;
                }

                if (updates->visible != storage_visible)
                {
                        storage_.set_visible(object.id(), updates->visible);
                }
        }
};
}
