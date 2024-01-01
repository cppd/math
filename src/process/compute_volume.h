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

#include <src/model/volume_object.h>
#include <src/progress/progress_list.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::process
{
template <std::size_t DIMENSION, std::size_t N>
void compute_slice(
        progress::RatioList* /*progress_list*/,
        const model::volume::VolumeObject<N>& volume_object,
        const std::vector<std::optional<int>>& slice_coordinates);
}
