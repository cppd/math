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

#pragma once

#include "obj.h"

#include "com/error.h"
#include "com/mat.h"
#include "com/mat_alg.h"
#include "com/types.h"

#include <tuple>
#include <unordered_set>
#include <vector>

namespace ObjAlgImplementation
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

        Vector<N, T> center = min + (max - min) / static_cast<T>(2);

        // Длину определять на самом большом типе с плавающей точкой,
        // так как Т может быть float и координаты точек иметь большие
        // для float величины, например, 10^30, что не позволяет считать
        // квадраты в скалярном произведении на float.
        T len = length(to_vector<long double>(max - min));

        if (!is_finite(center))
        {
                error("Object center is not finite");
        }
        if (!is_finite(len))
        {
                error("Object length is not finite");
        }
        if (!(len > 0))
        {
                error("Object length " + to_string(len) + " is not positive");
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

template <size_t N>
std::vector<int> unique_facet_indices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename Obj<N>::Facet& face : obj->facets())
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        vertices.insert(index);
                }
        }

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_line_indices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename Obj<N>::Line& line : obj->lines())
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        vertices.insert(index);
                }
        }

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_point_indices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename Obj<N>::Point& point : obj->points())
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(index);
        }

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_facet_vertices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename Obj<N>::Facet& face : obj->facets())
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        vertices.insert(obj->vertices()[index]);
                }
        }

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_line_vertices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename Obj<N>::Line& line : obj->lines())
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

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_point_vertices(const Obj<N>* obj)
{
        int vertex_count = obj->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename Obj<N>::Point& point : obj->points())
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(obj->vertices()[index]);
        }

        return ObjAlgImplementation::to_vector(vertices);
}

template <size_t N, typename T>
void center_and_length(const std::vector<Vector<N, T>>& vertices, const std::vector<typename Obj<N>::Facet>& facets,
                       Vector<N, T>* center, T* length)
{
        if (facets.size() < 1)
        {
                error("No facets");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min, max;

        ObjAlgImplementation::initial_min_max(&min, &max);

        for (const typename Obj<N>::Facet& facet : facets)
        {
                for (int index : facet.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        std::tie(*center, *length) = ObjAlgImplementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T>
void center_and_length(const std::vector<Vector<N, T>>& vertices, const std::vector<typename Obj<N>::Line>& lines,
                       Vector<N, T>* center, T* length)
{
        if (lines.size() < 1)
        {
                error("No lines");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min, max;

        ObjAlgImplementation::initial_min_max(&min, &max);

        for (const typename Obj<N>::Line& line : lines)
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

        std::tie(*center, *length) = ObjAlgImplementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T>
void center_and_length(const std::vector<Vector<N, T>>& vertices, const std::vector<typename Obj<N>::Point>& points,
                       Vector<N, T>* center, T* length)
{
        if (points.size() < 1)
        {
                error("No points");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min, max;

        ObjAlgImplementation::initial_min_max(&min, &max);

        for (const typename Obj<N>::Point& point : points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, vertices[index]);
                max = max_vector(max, vertices[index]);
        }

        std::tie(*center, *length) = ObjAlgImplementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T>
void min_max_coordinates(const std::vector<Vector<N, T>>& vertices, const std::vector<int>& indices, Vector<N, T>* min,
                         Vector<N, T>* max)
{
        int vertex_count = vertices.size();

        ObjAlgImplementation::initial_min_max(min, max);

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

template <size_t N>
Matrix<N + 1, N + 1, double> model_vertex_matrix(const Obj<N>* obj, double size, const Vector<N, double>& position)
{
        Matrix<N + 1, N + 1, double> m_to_center = translate(to_vector<double>(-obj->center()));
        Matrix<N + 1, N + 1, double> m_scale = scale(Vector<N, double>(size / obj->length()));
        Matrix<N + 1, N + 1, double> m_to_position = translate(position);
        return m_to_position * m_scale * m_to_center;
}
