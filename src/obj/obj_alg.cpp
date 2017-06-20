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

#include "obj_alg.h"

#include "com/error.h"
#include "com/hash.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <unordered_set>

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

std::vector<glm::vec3> get_unique_face_vertices(const IObj* obj)
{
        struct Hash
        {
                size_t operator()(glm::vec3 v) const
                {
                        return array_hash(std::array<float, 3>{{v[0], v[1], v[2]}});
                }
        };

        std::unordered_set<glm::vec3, Hash> unique_face_vertices(obj->get_vertices().size());

        for (const IObj::face3& face : obj->get_faces())
        {
                for (int i = 0; i < 3; ++i)
                {
                        unique_face_vertices.insert(obj->get_vertices()[face.vertices[i].v]);
                }
        }

        std::vector<glm::vec3> vertices;
        vertices.reserve(unique_face_vertices.size());

        for (glm::vec3 i : unique_face_vertices)
        {
                vertices.push_back(i);
        }

        return vertices;
}

void find_min_max(const std::vector<glm::vec3>& vertices, const std::vector<int>& indices, glm::vec3* min, glm::vec3* max)
{
        *min = glm::vec3(std::numeric_limits<float>::max());
        *max = glm::vec3(std::numeric_limits<float>::lowest());

        int max_index = static_cast<int>(vertices.size()) - 1;

        for (int i : indices)
        {
                if (i < 0 || i > max_index)
                {
                        error("vertex index out of bound");
                }

                *min = glm::min(*min, vertices[i]);
                *max = glm::max(*max, vertices[i]);
        }
}

void find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<IObj::face3> faces, glm::vec3* center,
                            float* length)
{
        std::vector<int> indices = get_unique_face_indices(faces);

        if (indices.size() < 3)
        {
                *center = glm::vec3(0);
                *length = 0;
                return;
        }

        glm::vec3 min, max;

        find_min_max(vertices, indices, &min, &max);

        *center = 0.5f * (max + min);

        *length = glm::length(max - min);
}
