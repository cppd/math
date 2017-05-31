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

#include "com/hash.h"

#include <unordered_set>

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
