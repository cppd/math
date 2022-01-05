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

#include <src/model/volume.h>

#include <memory>
#include <string>
#include <vector>

namespace ns::storage
{
template <std::size_t N>
struct VolumeObjectRepository
{
        virtual ~VolumeObjectRepository() = default;

        virtual std::vector<std::string> object_names() const = 0;
        virtual std::unique_ptr<volume::Volume<N>> object(const std::string& object_name, unsigned size) const = 0;
};

template <std::size_t N>
std::unique_ptr<VolumeObjectRepository<N>> create_volume_object_repository();
}
