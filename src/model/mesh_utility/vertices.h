/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../mesh.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::model::mesh
{
template <std::size_t N>
std::vector<Vector<N, float>> normalize_vertices(const Mesh<N>& mesh, const BoundingBox<N>& box)
{
        const Vector<N, float> extent = box.max - box.min;

        const float max_extent = extent.norm_infinity();

        if (max_extent == 0)
        {
                error("Mesh vertices are equal to each other");
        }

        const float scale_factor = 2 / max_extent;
        const Vector<N, float> center = box.min + 0.5f * extent;

        std::vector<Vector<N, float>> normalized_vertices;
        normalized_vertices.reserve(mesh.vertices.size());

        for (const Vector<N, float>& v : mesh.vertices)
        {
                const Vector<N, float> vertex = (v - center) * scale_factor;
                normalized_vertices.push_back(vertex);
        }

        return normalized_vertices;
}
}
