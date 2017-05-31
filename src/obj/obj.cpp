/*
Copyright (C) 2017 Topological Manifold

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

#include "obj.h"

#include "com/error.h"
#include "com/math.h"

#include <unordered_set>

namespace
{
std::vector<int> get_unique_face_indices(const std::vector<IObj::face3> faces)
{
        std::unordered_set<int> unique_face_vertices;

        for (const IObj::face3& face : faces)
        {
                for (int i = 0; i < 3; ++i)
                {
                        unique_face_vertices.insert(face.vertices[i].v);
                }
        }

        std::vector<int> indices;
        indices.reserve(unique_face_vertices.size());

        for (int i : unique_face_vertices)
        {
                indices.push_back(i);
        }

        return indices;
}
}

void IObj::find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<IObj::face3> faces, glm::vec3* center,
                                  float* length)
{
        std::vector<int> indices = get_unique_face_indices(faces);

        if (indices.size() < 3)
        {
                *center = glm::vec3(0);
                *length = 0;
                return;
        }

        float min_x, min_y, min_z, max_x, max_y, max_z;
        min_x = min_y = min_z = std::numeric_limits<float>::max();
        max_x = max_y = max_z = std::numeric_limits<float>::lowest();

        for (int i : indices)
        {
                if (i < 0 || i >= static_cast<int>(vertices.size()))
                {
                        error("vertex index out of bound");
                }

                glm::vec3 v = vertices[i];

                min_x = std::min(min_x, v.x);
                min_y = std::min(min_y, v.y);
                min_z = std::min(min_z, v.z);
                max_x = std::max(max_x, v.x);
                max_y = std::max(max_y, v.y);
                max_z = std::max(max_z, v.z);
        }

        *center = 0.5f * glm::vec3(max_x + min_x, max_y + min_y, max_z + min_z);
        *length = std::sqrt(square(max_x - min_x) + square(max_y - min_y) + square(max_z - min_z));
}
