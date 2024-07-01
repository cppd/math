/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "repository/mesh_objects.h"
#include "repository/volume_objects.h"

#include <src/com/sequence.h>
#include <src/model/mesh.h>
#include <src/model/volume.h>
#include <src/settings/dimensions.h>

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ns::storage
{
class Repository final
{
        template <std::size_t N>
        struct Repositories final
        {
                std::unique_ptr<const repository::MeshObjects<N>> meshes = repository::create_mesh_objects<N>();
                std::unique_ptr<const repository::VolumeObjects<N>> volumes = repository::create_volume_objects<N>();
        };

        using Tuple = Sequence<settings::Dimensions, std::tuple, Repositories>;

        Tuple data_;

public:
        struct ObjectNames final
        {
                int dimension;
                std::vector<std::string> point_mesh_names;
                std::vector<std::string> facet_mesh_names;
                std::vector<std::string> volume_names;
        };

        [[nodiscard]] std::vector<ObjectNames> object_names() const
        {
                std::vector<ObjectNames> names;

                const auto f = [&]<std::size_t N>(const Repositories<N>& v)
                {
                        names.resize(names.size() + 1);
                        names.back().dimension = N;
                        names.back().point_mesh_names = v.meshes->point_object_names();
                        names.back().facet_mesh_names = v.meshes->facet_object_names();
                        names.back().volume_names = v.volumes->object_names();
                };

                std::apply(
                        [&f](const auto&... v)
                        {
                                (f(v), ...);
                        },
                        data_);

                return names;
        }

        template <std::size_t N>
        [[nodiscard]] std::unique_ptr<model::mesh::Mesh<N>> point_mesh(
                const std::string& name,
                const unsigned point_count) const
        {
                return std::get<Repositories<N>>(data_).meshes->point_object(name, point_count);
        }

        template <std::size_t N>
        [[nodiscard]] std::unique_ptr<model::mesh::Mesh<N>> facet_mesh(
                const std::string& name,
                const unsigned facet_count) const
        {
                return std::get<Repositories<N>>(data_).meshes->facet_object(name, facet_count);
        }

        template <std::size_t N>
        [[nodiscard]] std::unique_ptr<model::volume::Volume<N>> volume(const std::string& name, const unsigned size)
                const
        {
                return std::get<Repositories<N>>(data_).volumes->object(name, size);
        }
};
}
