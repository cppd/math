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

#include "command.h"
#include "storage_mesh.h"
#include "storage_volume.h"

#include <src/com/error.h>

namespace ns::gpu::renderer
{
class RendererObject
{
        StorageMesh* mesh_storage_;
        StorageVolume* volume_storage_;

        void command(const MeshUpdate& v)
        {
                ASSERT(!volume_storage_->contains(v.object->id()));
                mesh_storage_->update(*v.object);
        }

        void command(const VolumeUpdate& v)
        {
                ASSERT(!mesh_storage_->contains(v.object->id()));
                volume_storage_->update(*v.object);
        }

        void command(const DeleteObject& v)
        {
                if (mesh_storage_->erase(v.id))
                {
                        ASSERT(!volume_storage_->contains(v.id));
                }
                else if (volume_storage_->erase(v.id))
                {
                        ASSERT(!mesh_storage_->contains(v.id));
                }
        }

        void command(const DeleteAllObjects&)
        {
                mesh_storage_->clear();
                volume_storage_->clear();
        }

public:
        RendererObject(StorageMesh* const mesh_storage, StorageVolume* const volume_storage)
                : mesh_storage_(mesh_storage),
                  volume_storage_(volume_storage)
        {
        }

        void command(const ObjectCommand& object_command)
        {
                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                std::visit(visitor, object_command);
        }
};
}
