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
#include "renderer_storage.h"

#include <src/com/log.h>

namespace ns::gpu::renderer
{
class RendererStorageMeshEvents : public RendererStorageEvents<MeshObject>
{
        virtual void mesh_visibility_changed() = 0;

        void visibility_changed() final
        {
                mesh_visibility_changed();
        }

protected:
        ~RendererStorageMeshEvents() = default;

public:
        virtual std::unique_ptr<MeshObject> create_mesh() const = 0;
        virtual void mesh_changed(const MeshObject::UpdateChanges& update_changes) = 0;
};

class RendererStorageMesh final
{
        RendererStorage<MeshObject, const MeshObject> storage_;
        RendererStorageMeshEvents* events_;

public:
        explicit RendererStorageMesh(RendererStorageMeshEvents* const events) : storage_(events), events_(events)
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

        void update(const mesh::MeshObject<3>& object)
        {
                MeshObject* const ptr = [&]
                {
                        MeshObject* const p = storage_.object(object.id());
                        if (p)
                        {
                                return p;
                        }
                        return storage_.insert(object.id(), events_->create_mesh());
                }();

                MeshObject::UpdateChanges update_changes;
                bool visible;

                try
                {
                        const mesh::Reading reading(object);
                        visible = reading.visible();
                        update_changes = ptr->update(reading);
                }
                catch (const std::exception& e)
                {
                        storage_.erase(object.id());
                        LOG(std::string("Error updating mesh object. ") + e.what());
                        return;
                }
                catch (...)
                {
                        storage_.erase(object.id());
                        LOG("Unknown error updating mesh object");
                        return;
                }

                const bool storage_visible = storage_.is_visible(object.id());

                if (visible && storage_visible)
                {
                        events_->mesh_changed(update_changes);
                        return;
                }

                if (visible || storage_visible)
                {
                        storage_.set_visible(object.id(), visible);
                }
        }
};
}
