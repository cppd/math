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

#include "bounding_box.h"

#include <src/com/error.h>
#include <src/model/com/functions.h>
#include <src/model/volume.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>
#include <tuple>

namespace ns::model::volume
{
template <std::size_t N>
std::tuple<numerical::Vector<N, double>, double> center_and_length(const Volume<N>& volume)
{
        const std::optional<BoundingBox<N>> box = bounding_box(volume);
        if (!box)
        {
                error("Volume has no geometry");
        }
        return com::center_and_length_for_min_max(box->min, box->max);
}
}
