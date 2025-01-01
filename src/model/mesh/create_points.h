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

#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace ns::model::mesh
{
template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_points(const std::vector<numerical::Vector<N, float>>& points);

template <std::size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_points(std::vector<numerical::Vector<N, float>>&& points);
}
