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

#include <src/com/sequence.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/settings/dimensions.h>

#include <cstddef>
#include <memory>
#include <variant>

namespace ns::storage
{
namespace types_implementation
{
template <std::size_t N>
using MeshObjectPtr = std::shared_ptr<model::mesh::MeshObject<N>>;
template <std::size_t N>
using MeshObjectWeakPtr = std::weak_ptr<model::mesh::MeshObject<N>>;

template <std::size_t N>
using MeshObjectConstPtr = std::shared_ptr<const model::mesh::MeshObject<N>>;

template <std::size_t N>
using VolumeObjectPtr = std::shared_ptr<model::volume::VolumeObject<N>>;
template <std::size_t N>
using VolumeObjectWeakPtr = std::weak_ptr<model::volume::VolumeObject<N>>;

template <std::size_t N>
using VolumeObjectConstPtr = std::shared_ptr<const model::volume::VolumeObject<N>>;
}

using MeshObject = Sequence<settings::Dimensions, std::variant, types_implementation::MeshObjectPtr>;
using MeshObjectWeak = Sequence<settings::Dimensions, std::variant, types_implementation::MeshObjectWeakPtr>;
using MeshObjectConst = Sequence<settings::Dimensions, std::variant, types_implementation::MeshObjectConstPtr>;

using VolumeObject = Sequence<settings::Dimensions, std::variant, types_implementation::VolumeObjectPtr>;
using VolumeObjectWeak = Sequence<settings::Dimensions, std::variant, types_implementation::VolumeObjectWeakPtr>;
using VolumeObjectConst = Sequence<settings::Dimensions, std::variant, types_implementation::VolumeObjectConstPtr>;
}
