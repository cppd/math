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

#include "repository/meshes.h"
#include "repository/volumes.h"

#include <src/com/sequence.h>
#include <src/settings/dimensions.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ns::storage
{
class Repository final
{
        template <std::size_t N>
        struct Repositories
        {
                std::unique_ptr<const MeshObjectRepository<N>> meshes;
                std::unique_ptr<const VolumeObjectRepository<N>> volumes;
                Repositories()
                        : meshes(create_mesh_object_repository<N>()), volumes(create_volume_object_repository<N>())
                {
                }
        };

        using Tuple = Sequence<settings::Dimensions, std::tuple, Repositories>;

        Tuple m_data;

public:
        struct ObjectNames
        {
                int dimension;
                std::vector<std::string> point_mesh_names;
                std::vector<std::string> facet_mesh_names;
                std::vector<std::string> volume_names;
        };

        std::vector<ObjectNames> object_names() const
        {
                std::vector<ObjectNames> names;

                std::apply(
                        [&]<std::size_t... N>(const Repositories<N>&... v)
                        {
                                (
                                        [&]()
                                        {
                                                names.resize(names.size() + 1);
                                                names.back().dimension = N;
                                                names.back().point_mesh_names = v.meshes->point_object_names();
                                                names.back().facet_mesh_names = v.meshes->facet_object_names();
                                                names.back().volume_names = v.volumes->object_names();
                                        }(),
                                        ...);
                        },
                        m_data);

                return names;
        }

        template <std::size_t N>
        std::unique_ptr<mesh::Mesh<N>> point_mesh(const std::string& name, unsigned point_count) const
        {
                return std::get<Repositories<N>>(m_data).meshes->point_object(name, point_count);
        }

        template <std::size_t N>
        std::unique_ptr<mesh::Mesh<N>> facet_mesh(const std::string& name, unsigned facet_count) const
        {
                return std::get<Repositories<N>>(m_data).meshes->facet_object(name, facet_count);
        }

        template <std::size_t N>
        std::unique_ptr<volume::Volume<N>> volume(const std::string& name, unsigned size) const
        {
                return std::get<Repositories<N>>(m_data).volumes->object(name, size);
        }
};
}
