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

#include "options.h"
#include "repository.h"

#include <src/com/sequence.h>

#include <set>
#include <string>
#include <tuple>

namespace storage
{
class MultiRepository final
{
        // std::tuple<T<MIN>, ..., T<MAX>>.
        using Tuple = SequenceType1<std::tuple, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, Repository>;

        Tuple m_data;

public:
        std::set<unsigned> supported_dimensions()
        {
                std::set<unsigned> v;
                for (int d = MINIMUM_DIMENSION; d <= MAXIMUM_DIMENSION; ++d)
                {
                        v.insert(d);
                }
                return v;
        }

        template <size_t N>
        const Repository<N>& repository() const
        {
                static_assert(N >= MINIMUM_DIMENSION && N <= MAXIMUM_DIMENSION);
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
                        [&](const auto&... v) {
                                (
                                        [&]() {
                                                names.resize(names.size() + 1);
                                                names.back().dimension = v.DIMENSION;
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
