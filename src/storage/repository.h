/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "repository/meshes.h"
#include "repository/volumes.h"

template <size_t N>
class Repository final
{
        std::unique_ptr<const MeshObjectRepository<N>> m_mesh_object_repository;
        std::unique_ptr<const VolumeObjectRepository<N>> m_volume_object_repository;

public:
        static constexpr size_t DIMENSION = N;

        Repository()
                : m_mesh_object_repository(create_mesh_object_repository<N>()),
                  m_volume_object_repository(create_volume_object_repository<N>())
        {
        }

        const MeshObjectRepository<N>& meshes() const
        {
                return *m_mesh_object_repository;
        }

        const VolumeObjectRepository<N>& volumes() const
        {
                return *m_volume_object_repository;
        }
};
