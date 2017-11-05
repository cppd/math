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
#include "com/mat_alg.h"
#include "com/mat_glm.h"
#include "com/vec_glm.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <unordered_set>

namespace
{
struct Hash
{
        size_t operator()(glm::vec3 v) const
        {
                return array_hash(std::array<float, 3>{{v[0], v[1], v[2]}});
        }
};
}

std::vector<int> get_unique_face_indices(const std::vector<IObj::face3>& faces)
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

std::vector<int> get_unique_point_indices(const std::vector<int>& points)
{
        std::unordered_set<int> unique_point_vertices;

        for (int point : points)
        {
                unique_point_vertices.insert(point);
        }

        std::vector<int> indices;
        indices.reserve(unique_point_vertices.size());

        for (int i : unique_point_vertices)
        {
                indices.push_back(i);
        }

        return indices;
}

std::vector<glm::vec3> get_unique_face_vertices(const IObj* obj)
{
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

std::vector<glm::vec3> get_unique_point_vertices(const IObj* obj)
{
        std::unordered_set<glm::vec3, Hash> unique_point_vertices(obj->get_points().size());

        for (int point : obj->get_points())
        {
                unique_point_vertices.insert(obj->get_vertices()[point]);
        }

        std::vector<glm::vec3> vertices;
        vertices.reserve(unique_point_vertices.size());

        for (glm::vec3 i : unique_point_vertices)
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

void find_min_max(const std::vector<glm::vec3>& vertices, glm::vec3* min, glm::vec3* max)
{
        *min = glm::vec3(std::numeric_limits<float>::max());
        *max = glm::vec3(std::numeric_limits<float>::lowest());

        for (const glm::vec3& v : vertices)
        {
                *min = glm::min(*min, v);
                *max = glm::max(*max, v);
        }
}

namespace
{
void center_and_length(const std::vector<glm::vec3>& vertices, std::vector<int>& indices, glm::vec3* center, float* length)
{
        glm::vec3 min, max;

        find_min_max(vertices, indices, &min, &max);

        *center = 0.5f * (max + min);

        *length = glm::length(max - min);
}

void center_and_length(const std::vector<glm::vec3>& vertices, glm::vec3* center, float* length)
{
        glm::vec3 min, max;

        find_min_max(vertices, &min, &max);

        *center = 0.5f * (max + min);

        *length = glm::length(max - min);
}
}

void find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<IObj::face3>& faces, glm::vec3* center,
                            float* length)
{
        std::vector<int> indices = get_unique_face_indices(faces);

        if (indices.size() < 3)
        {
                error("face unique indices count < 3");
        }

        center_and_length(vertices, indices, center, length);
}

void find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<int>& points, glm::vec3* center,
                            float* length)
{
        std::vector<int> indices = get_unique_point_indices(points);

        if (indices.size() < 2)
        {
                error("points unique indices count < 2");
        }

        center_and_length(vertices, indices, center, length);
}

void find_center_and_length(const std::vector<glm::vec3>& vertices, glm::vec3* center, float* length)
{
        if (vertices.size() < 2)
        {
                error("points count < 2");
        }

        center_and_length(vertices, center, length);
}

glm::dmat4 get_model_vertex_matrix(const IObj* obj, double size, const glm::dvec3& position)
{
        mat4 m_to_center = translate(to_vector<double>(-obj->get_center()));
        mat4 m_scale = scale(vec3(size / obj->get_length()));
        mat4 m_to_position = translate(to_vector<double>(position));
        return to_glm<double>(m_to_position * m_scale * m_to_center);
}
