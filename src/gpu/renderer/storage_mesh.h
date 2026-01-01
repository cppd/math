/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "mesh/object.h"

#include <src/com/log.h>
#include <src/model/mesh_object.h>
#include <src/model/object_id.h>

#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace ns::gpu::renderer
{
class StorageMeshEvents
{
protected:
        ~StorageMeshEvents() = default;

public:
        virtual std::unique_ptr<MeshObject> mesh_create() = 0;
        virtual void mesh_visibility_changed() = 0;
        virtual void mesh_visible_changed(const MeshObject::UpdateChanges& update_changes) = 0;
};

class StorageMesh final : private StorageEvents<MeshObject>
{
        Storage<MeshObject, const MeshObject> storage_;
        StorageMeshEvents* events_;

        void visibility_changed() override
        {
                events_->mesh_visibility_changed();
        }

        struct Updates final
        {
                bool visible;
                MeshObject::UpdateChanges changes;
        };

        std::optional<Updates> update_mesh(const model::mesh::MeshObject<3>& object)
        {
                MeshObject* const ptr = [&]
                {
                        if (MeshObject* const p = storage_.object(object.id()))
                        {
                                return p;
                        }
                        return storage_.insert(object.id(), events_->mesh_create());
                }();

                try
                {
                        const model::mesh::Reading reading(object);
                        return Updates{.visible = reading.visible(), .changes = ptr->update(reading)};
                }
                catch (const std::exception& e)
                {
                        storage_.erase(object.id());
                        LOG(std::string("Error updating mesh object: ") + e.what());
                        return std::nullopt;
                }
                catch (...)
                {
                        storage_.erase(object.id());
                        LOG("Unknown error updating mesh object");
                        return std::nullopt;
                }
        }

public:
        explicit StorageMesh(StorageMeshEvents* const events)
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

        void update(const model::mesh::MeshObject<3>& object)
        {
                const std::optional<Updates> updates = update_mesh(object);

                if (!updates)
                {
                        return;
                }

                const bool storage_visible = storage_.is_visible(object.id());

                if (updates->visible && storage_visible)
                {
                        events_->mesh_visible_changed(updates->changes);
                        return;
                }

                if (updates->visible != storage_visible)
                {
                        storage_.set_visible(object.id(), updates->visible);
                }
        }
};
}
