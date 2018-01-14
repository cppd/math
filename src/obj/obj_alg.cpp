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

template <typename T>
std::vector<T> to_vector(const std::unordered_set<T>& set)
{
        std::vector<T> vector;
        vector.reserve(set.size());
        for (const T& v : set)
        {
                vector.push_back(v);
        }
        return vector;
}

template <size_t N, typename T>
void initial_min_max(Vector<N, T>* min, Vector<N, T>* max)
{
        *min = Vector<N, T>(limits<T>::max());
        *max = Vector<N, T>(limits<T>::lowest());
}
}

std::vector<int> unique_face_indices(const std::vector<IObj::face3>& faces)
{
        std::unordered_set<int> indices(faces.size());

        for (const IObj::face3& face : faces)
        {
                for (const IObj::vertex& vertex : face.vertices)
                {
                        int index = vertex.v;
                        indices.insert(index);
                }
        }

        return to_vector(indices);
}

std::vector<int> unique_line_indices(const std::vector<IObj::line>& lines)
{
        std::unordered_set<int> indices(lines.size());

        for (const IObj::line& line : lines)
        {
                for (int index : line.vertices)
                {
                        indices.insert(index);
                }
        }

        return to_vector(indices);
}

std::vector<int> unique_point_indices(const std::vector<int>& points)
{
        std::unordered_set<int> indices(points.size());

        for (int index : points)
        {
                indices.insert(index);
        }

        return to_vector(indices);
}

std::vector<vec3f> unique_face_vertices(const IObj* obj)
{
        int vertex_count = obj->get_vertices().size();

        std::unordered_set<vec3f> vertices(obj->get_vertices().size());

        for (const IObj::face3& face : obj->get_faces())
        {
                for (const IObj::vertex& vertex : face.vertices)
                {
                        int index = vertex.v;

                        if (index < 0 || index >= vertex_count)
                        {
                                error("Face vertex index out of bounds");
                        }

                        vertices.insert(obj->get_vertices()[index]);
                }
        }

        return to_vector(vertices);
}

std::vector<vec3f> unique_line_vertices(const IObj* obj)
{
        int vertex_count = obj->get_vertices().size();

        std::unordered_set<vec3f> vertices(obj->get_lines().size());

        for (const IObj::line& line : obj->get_lines())
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        vertices.insert(obj->get_vertices()[index]);
                }
        }

        return to_vector(vertices);
}

std::vector<vec3f> unique_point_vertices(const IObj* obj)
{
        int vertex_count = obj->get_vertices().size();

        std::unordered_set<vec3f> vertices(obj->get_points().size());

        for (int index : obj->get_points())
        {
                if (index < 0 || index >= vertex_count)
                {
                        error("Vertex index out of bounds");
                }

                vertices.insert(obj->get_vertices()[index]);
        }

        return to_vector(vertices);
}

void min_max_coordinates(const std::vector<vec3f>& vertices, const std::vector<int>& indices, vec3f* min, vec3f* max)
{
        int vertex_count = vertices.size();

        initial_min_max(min, max);

        for (int index : indices)
        {
                if (index < 0 || index >= vertex_count)
                {
                        error("Vertex index out of bounds");
                }

                *min = min_vector(*min, vertices[index]);
                *max = max_vector(*max, vertices[index]);
        }
}

void min_max_coordinates(const std::vector<vec3f>& vertices, vec3f* min, vec3f* max)
{
        initial_min_max(min, max);

        for (const vec3f& v : vertices)
        {
                *min = min_vector(*min, v);
                *max = max_vector(*max, v);
        }
}

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::face3>& faces, vec3f* center, float* length)
{
        if (faces.size() < 1)
        {
                error("No faces");
        }

        int vertex_count = vertices.size();

        vec3f min, max;

        initial_min_max(&min, &max);

        for (const IObj::face3& face : faces)
        {
                for (const IObj::vertex& vertex : face.vertices)
                {
                        int index = vertex.v;

                        if (index < 0 || index >= vertex_count)
                        {
                                error("Face vertex index out of bounds");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::line>& lines, vec3f* center, float* length)
{
        if (lines.size() < 1)
        {
                error("No lines");
        }

        int vertex_count = vertices.size();

        vec3f min, max;

        initial_min_max(&min, &max);

        for (const IObj::line& line : lines)
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<int>& points, vec3f* center, float* length)
{
        if (points.size() < 1)
        {
                error("No points");
        }

        vec3f min, max;

        min_max_coordinates(vertices, points, &min, &max);

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

void center_and_length(const std::vector<vec3f>& vertices, vec3f* center, float* length)
{
        if (vertices.size() < 1)
        {
                error("No vertices");
        }

        vec3f min, max;

        min_max_coordinates(vertices, &min, &max);

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
}

mat4 model_vertex_matrix(const IObj* obj, double size, const vec3& position)
{
        mat4 m_to_center = translate(to_vector<double>(-obj->get_center()));
        mat4 m_scale = scale(vec3(size / obj->get_length()));
        mat4 m_to_position = translate(position);
        return m_to_position * m_scale * m_to_center;
}
