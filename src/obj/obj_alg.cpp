/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include "com/mat_alg.h"
#include "com/types.h"

#include <unordered_set>

namespace
{
template <size_t N, typename T>
std::tuple<Vector<N, T>, T> center_and_length_for_min_max(const Vector<N, T>& min, const Vector<N, T>& max)
{
        static_assert(is_floating_point<T>);

        for (unsigned i = 0; i < N; ++i)
        {
                if (min[i] >= max[i])
                {
                        error("Object size error");
                }
        }

        Vector<N, T> center = (max + min) / static_cast<T>(2);

        T len = length(max - min);

        if (len <= 0)
        {
                error("Object length is not positive");
        }

        return {center, len};
}
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

std::vector<int> get_unique_line_indices(const std::vector<std::array<int, 2>>& lines)
{
        std::unordered_set<int> unique_line_vertices;

        for (const std::array<int, 2>& line : lines)
        {
                unique_line_vertices.insert(line[0]);
                unique_line_vertices.insert(line[1]);
        }

        std::vector<int> indices;
        indices.reserve(unique_line_vertices.size());

        for (int i : unique_line_vertices)
        {
                indices.push_back(i);
        }

        return indices;
}

std::vector<vec3f> get_unique_face_vertices(const IObj* obj)
{
        std::unordered_set<vec3f> unique_face_vertices(obj->get_vertices().size());

        for (const IObj::face3& face : obj->get_faces())
        {
                for (int i = 0; i < 3; ++i)
                {
                        unique_face_vertices.insert(obj->get_vertices()[face.vertices[i].v]);
                }
        }

        std::vector<vec3f> vertices;
        vertices.reserve(unique_face_vertices.size());

        for (vec3f i : unique_face_vertices)
        {
                vertices.push_back(i);
        }

        return vertices;
}

std::vector<vec3f> get_unique_point_vertices(const IObj* obj)
{
        std::unordered_set<vec3f> unique_point_vertices(obj->get_points().size());

        for (int point : obj->get_points())
        {
                unique_point_vertices.insert(obj->get_vertices()[point]);
        }

        std::vector<vec3f> vertices;
        vertices.reserve(unique_point_vertices.size());

        for (vec3f i : unique_point_vertices)
        {
                vertices.push_back(i);
        }

        return vertices;
}

void find_min_max(const std::vector<vec3f>& vertices, const std::vector<int>& indices, vec3f* min, vec3f* max)
{
        int max_index = static_cast<int>(vertices.size()) - 1;

        *min = vec3f(std::numeric_limits<float>::max());
        *max = vec3f(std::numeric_limits<float>::lowest());

        for (int index : indices)
        {
                if (index < 0 || index > max_index)
                {
                        error("Vertex index out of bound");
                }

                *min = min_vector(*min, vertices[index]);
                *max = max_vector(*max, vertices[index]);
        }
}

void find_min_max(const std::vector<vec3f>& vertices, vec3f* min, vec3f* max)
{
        *min = vec3f(std::numeric_limits<float>::max());
        *max = vec3f(std::numeric_limits<float>::lowest());

        for (const vec3f& v : vertices)
        {
                *min = min_vector(*min, v);
                *max = max_vector(*max, v);
        }
}

void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::face3>& faces, vec3f* center,
                            float* length)
{
        if (faces.size() < 1)
        {
                error("No faces");
        }

        int max_index = static_cast<int>(vertices.size()) - 1;

        vec3f min = vec3f(std::numeric_limits<float>::max());
        vec3f max = vec3f(std::numeric_limits<float>::lowest());

        for (const IObj::face3& face : faces)
        {
                for (const IObj::vertex& vertex : face.vertices)
                {
                        int index = vertex.v;

                        if (index < 0 || index > max_index)
                        {
                                error("Face vertex index out of bound");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<std::array<int, 2>>& lines, vec3f* center,
                            float* length)
{
        if (lines.size() < 1)
        {
                error("No lines");
        }

        int max_index = static_cast<int>(vertices.size()) - 1;

        vec3f min = vec3f(std::numeric_limits<float>::max());
        vec3f max = vec3f(std::numeric_limits<float>::lowest());

        for (const std::array<int, 2>& line : lines)
        {
                for (int index : line)
                {
                        if (index < 0 || index > max_index)
                        {
                                error("Line vertex index out of bound");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void find_center_and_length(const std::vector<vec3f>& vertices, const std::vector<int>& points, vec3f* center, float* length)
{
        if (points.size() < 1)
        {
                error("No points");
        }

        vec3f min, max;

        find_min_max(vertices, points, &min, &max);

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void find_center_and_length(const std::vector<vec3f>& vertices, vec3f* center, float* length)
{
        if (vertices.size() < 1)
        {
                error("No vertices");
        }

        vec3f min, max;

        find_min_max(vertices, &min, &max);

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

mat4 get_model_vertex_matrix(const IObj* obj, double size, const vec3& position)
{
        mat4 m_to_center = translate(to_vector<double>(-obj->get_center()));
        mat4 m_scale = scale(vec3(size / obj->get_length()));
        mat4 m_to_position = translate(position);
        return m_to_position * m_scale * m_to_center;
}
