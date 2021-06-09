/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "shape.h"

#include <src/model/mesh_object.h>
#include <src/progress/progress.h>

#include <memory>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
std::unique_ptr<Shape<N, T, Color>> create_mesh(
        const std::vector<const mesh::MeshObject<N>*>& mesh_objects,
        ProgressRatio* progress);
}
