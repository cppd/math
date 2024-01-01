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

#include "../../mesh.h"

#include <src/progress/progress.h>

#include <bit>
#include <cstddef>
#include <memory>

namespace ns::model::mesh::file
{
template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_stl_file(
        const Path& file_name,
        progress::Ratio* progress,
        bool byte_swap = (std::endian::native == std::endian::big));
}
