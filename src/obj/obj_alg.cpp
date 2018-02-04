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

std::vector<int> unique_face_indices(const std::vector<IObj::Face>& faces)
{
        std::unordered_set<int> indices(faces.size());

        for (const IObj::Face& face : faces)
        {
                for (int index : face.vertices)
                {
                        indices.insert(index);
                }
        }

        return to_vector(indices);
}

std::vector<int> unique_line_indices(const std::vector<IObj::Line>& lines)
{
        std::unordered_set<int> indices(lines.size());

        for (const IObj::Line& line : lines)
        {
                for (int index : line.vertices)
                {
                        indices.insert(index);
                }
        }

        return to_vector(indices);
}

std::vector<int> unique_point_indices(const std::vector<IObj::Point>& points)
{
        std::unordered_set<int> indices(points.size());

        for (const IObj::Point& point : points)
        {
                indices.insert(point.vertex);
        }

        return to_vector(indices);
}

std::vector<vec3f> unique_face_vertices(const IObj* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<vec3f> vertices(vertex_count);

        for (const IObj::Face& face : obj->faces())
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Face vertex index out of bounds");
                        }

                        vertices.insert(obj->vertices()[index]);
                }
        }

        return to_vector(vertices);
}

std::vector<vec3f> unique_line_vertices(const IObj* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<vec3f> vertices(vertex_count);

        for (const IObj::Line& line : obj->lines())
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        vertices.insert(obj->vertices()[index]);
                }
        }

        return to_vector(vertices);
}

std::vector<vec3f> unique_point_vertices(const IObj* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<vec3f> vertices(vertex_count);

        for (const IObj::Point& point : obj->points())
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(obj->vertices()[index]);
        }

        return to_vector(vertices);
}

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::Face>& faces, vec3f* center, float* length)
{
        if (faces.size() < 1)
        {
                error("No faces");
        }

        int vertex_count = vertices.size();

        vec3f min, max;

        initial_min_max(&min, &max);

        for (const IObj::Face& face : faces)
        {
                for (int index : face.vertices)
                {
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

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::Line>& lines, vec3f* center, float* length)
{
        if (lines.size() < 1)
        {
                error("No lines");
        }

        int vertex_count = vertices.size();

        vec3f min, max;

        initial_min_max(&min, &max);

        for (const IObj::Line& line : lines)
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

void center_and_length(const std::vector<vec3f>& vertices, const std::vector<IObj::Point>& points, vec3f* center, float* length)
{
        if (points.size() < 1)
        {
                error("No points");
        }

        int vertex_count = vertices.size();

        vec3f min, max;

        initial_min_max(&min, &max);

        for (const IObj::Point& point : points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, vertices[index]);
                max = max_vector(max, vertices[index]);
        }

        std::tie(*center, *length) = center_and_length_for_min_max(min, max);
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

mat4 model_vertex_matrix(const IObj* obj, double size, const vec3& position)
{
        mat4 m_to_center = translate(to_vector<double>(-obj->center()));
        mat4 m_scale = scale(vec3(size / obj->length()));
        mat4 m_to_position = translate(position);
        return m_to_position * m_scale * m_to_center;
}
