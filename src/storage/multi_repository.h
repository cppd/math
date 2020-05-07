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

#include "repository.h"

#include <src/com/sequence.h>
#include <src/settings/dimensions.h>

#include <set>
#include <string>
#include <tuple>

namespace storage
{
class MultiRepository final
{
        using Tuple = Sequence<settings::Dimensions, std::tuple, Repository>;

        Tuple m_data;

public:
        template <size_t N>
        const Repository<N>& repository() const
        {
                return std::get<Repository<N>>(m_data);
        }

        struct ObjectNames
        {
                int dimension;
                std::vector<std::string> mesh_names;
                std::vector<std::string> volume_names;
        };

        std::vector<ObjectNames> object_names() const
        {
                std::vector<ObjectNames> names;

                std::apply(
                        [&]<size_t... N>(const Repository<N>&... v) {
                                (
                                        [&]() {
                                                names.resize(names.size() + 1);
                                                names.back().dimension = N;
                                                names.back().mesh_names = v.meshes().object_names();
                                                names.back().volume_names = v.volumes().object_names();
                                        }(),
                                        ...);
                        },
                        m_data);

                return names;
        }
};
}
