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

#include <src/model/mesh.h>
#include <src/numerical/vec.h>

#include <array>
#include <memory>

namespace mesh
{
template <size_t N>
std::unique_ptr<MeshModel<N>> create_mesh_for_lines(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, 2>>& lines);
}
