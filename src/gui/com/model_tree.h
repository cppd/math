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

#include <src/model/object_id.h>
#include <src/storage/types.h>

#include <optional>

namespace ns::gui::com
{
class ModelTreeEvents
{
protected:
        ~ModelTreeEvents() = default;

public:
        virtual void insert(storage::MeshObject&& object, const std::optional<model::ObjectId>& parent_object_id) = 0;
        virtual void insert(storage::VolumeObject&& object, const std::optional<model::ObjectId>& parent_object_id) = 0;
        virtual void update(storage::MeshObjectWeak&& object) = 0;
        virtual void update(storage::VolumeObjectWeak&& object) = 0;
        virtual void erase(model::ObjectId id) = 0;
};
}
