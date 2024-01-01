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

#include "bounding_box.h"

#include "../com/functions.h"
#include "../mesh.h"

#include <src/com/error.h>

#include <cstddef>
#include <optional>

namespace ns::model::mesh
{
template <std::size_t N>
void set_center_and_length(Mesh<N>* const mesh)
{
        ASSERT(mesh);

        std::optional<BoundingBox<N>> box = bounding_box(*mesh);
        if (!box)
        {
                error("Mesh has no geometry");
        }
        std::tie(mesh->center, mesh->length) = center_and_length_for_min_max(box->min, box->max);
}
}
