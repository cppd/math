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

#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::painter::shapes::mesh
{
template <std::size_t N, typename T>
[[nodiscard]] model::mesh::Mesh<N> optimize_mesh(
        const model::mesh::Reading<N>& mesh_object,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation);
}
