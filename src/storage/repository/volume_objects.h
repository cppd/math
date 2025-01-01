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

#include <src/model/volume.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ns::storage::repository
{
template <std::size_t N>
class VolumeObjects
{
public:
        virtual ~VolumeObjects() = default;

        [[nodiscard]] virtual std::vector<std::string> object_names() const = 0;

        [[nodiscard]] virtual std::unique_ptr<model::volume::Volume<N>> object(
                const std::string& object_name,
                unsigned size) const = 0;
};

template <std::size_t N>
[[nodiscard]] std::unique_ptr<const VolumeObjects<N>> create_volume_objects();
}
