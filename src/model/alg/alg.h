/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../mesh.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/com/type/trait.h>
#include <src/numerical/transform.h>

#include <algorithm>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace mesh_model_alg_implementation
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

        // Т может быть float и координаты точек могут иметь большие
        // для float величины, например, 10^30, что не позволяет считать
        // квадраты на float, поэтому используется функция norm_stable.
        T len = (max - min).norm_stable();

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
std::vector<int> unique_facet_indices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Facet& face : mesh->facets())
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

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_line_indices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Line& line : mesh->lines())
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

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_point_indices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Point& point : mesh->points())
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(index);
        }

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_facet_vertices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Facet& face : mesh->facets())
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        vertices.insert(mesh->vertices()[index]);
                }
        }

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_line_vertices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Line& line : mesh->lines())
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        vertices.insert(mesh->vertices()[index]);
                }
        }

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_point_vertices(const MeshModel<N>* mesh)
{
        int vertex_count = mesh->vertices().size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Point& point : mesh->points())
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(mesh->vertices()[index]);
        }

        return mesh_model_alg_implementation::to_vector(vertices);
}

template <size_t N, typename T>
void center_and_length(
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<typename MeshModel<N>::Facet>& facets,
        Vector<N, T>* center,
        T* length)
{
        if (facets.empty())
        {
                error("No facets");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min;
        Vector<N, T> max;

        mesh_model_alg_implementation::initial_min_max(&min, &max);

        for (const typename MeshModel<N>::Facet& facet : facets)
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

        std::tie(*center, *length) = mesh_model_alg_implementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T>
void center_and_length(
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<typename MeshModel<N>::Line>& lines,
        Vector<N, T>* center,
        T* length)
{
        if (lines.empty())
        {
                error("No lines");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min;
        Vector<N, T> max;

        mesh_model_alg_implementation::initial_min_max(&min, &max);

        for (const typename MeshModel<N>::Line& line : lines)
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

        std::tie(*center, *length) = mesh_model_alg_implementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T>
void center_and_length(
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<typename MeshModel<N>::Point>& points,
        Vector<N, T>* center,
        T* length)
{
        if (points.empty())
        {
                error("No points");
        }

        int vertex_count = vertices.size();

        Vector<N, T> min;
        Vector<N, T> max;

        mesh_model_alg_implementation::initial_min_max(&min, &max);

        for (const typename MeshModel<N>::Point& point : points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, vertices[index]);
                max = max_vector(max, vertices[index]);
        }

        std::tie(*center, *length) = mesh_model_alg_implementation::center_and_length_for_min_max(min, max);
}

template <size_t N, typename T, typename... Indices>
std::tuple<Vector<N, T>, Vector<N, T>> min_max_coordinates(
        const std::vector<Vector<N, T>>& vertices,
        const Indices&... indices)
{
        static_assert((std::is_same_v<Indices, std::vector<int>> && ...));
        static_assert(sizeof...(Indices) > 0);

        if ((indices.empty() && ...))
        {
                error("No indices");
        }

        std::array<const std::vector<int>*, sizeof...(Indices)> pointers{&indices...};

        int vertex_count = vertices.size();

        Vector<N, T> min;
        Vector<N, T> max;

        mesh_model_alg_implementation::initial_min_max(&min, &max);

        for (const std::vector<int>* ptr : pointers)
        {
                for (int index : *ptr)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Vertex index out of bounds");
                        }

                        min = min_vector(min, vertices[index]);
                        max = max_vector(max, vertices[index]);
                }
        }

        return {min, max};
}

template <size_t N>
Matrix<N + 1, N + 1, double> model_vertex_matrix(
        const MeshModel<N>& mesh,
        double size,
        const Vector<N, double>& position)
{
        Matrix<N + 1, N + 1, double> m_to_center = translate(to_vector<double>(-mesh.center()));
        Matrix<N + 1, N + 1, double> m_scale = scale(Vector<N, double>(size / mesh.length()));
        Matrix<N + 1, N + 1, double> m_to_position = translate(position);
        return m_to_position * m_scale * m_to_center;
}

template <size_t N>
void sort_facets_by_material(
        const MeshModel<N>& mesh,
        std::vector<int>* sorted_facet_indices,
        std::vector<int>* facet_offset,
        std::vector<int>* facet_count)
{
        ASSERT(std::all_of(
                std::cbegin(mesh.facets()), std::cend(mesh.facets()), [&](const typename MeshModel<N>::Facet& facet) {
                        return facet.material < static_cast<int>(mesh.materials().size());
                }));

        // Robert Sedgewick, Kevin Wayne.
        // Algorithms. Fourth edition.
        // Pearson Education, 2011.
        // 5.1 String Sorts
        // Key-indexed counting

        // Для граней без материала используется дополнительный материал в конце

        int max_material_index = mesh.materials().size();
        int new_material_size = mesh.materials().size() + 1;

        auto material_index = [&](int i) { return i >= 0 ? i : max_material_index; };

        // Количество граней с заданным материалом
        *facet_count = std::vector<int>(new_material_size, 0);

        for (const typename MeshModel<N>::Facet& facet : mesh.facets())
        {
                int m = material_index(facet.material);
                ++(*facet_count)[m];
        }

        // Начала расположения граней с заданным материалом
        *facet_offset = std::vector<int>(new_material_size);

        for (int i = 0, sum = 0; i < new_material_size; ++i)
        {
                (*facet_offset)[i] = sum;
                sum += (*facet_count)[i];
        }

        // Индексы граней по возрастанию их материала
        sorted_facet_indices->resize(mesh.facets().size());

        // Текущие начала расположения граней с заданным материалом
        std::vector<int> starting_indices = *facet_offset;
        for (size_t i = 0; i < mesh.facets().size(); ++i)
        {
                int m = material_index(mesh.facets()[i].material);
                (*sorted_facet_indices)[starting_indices[m]++] = i;
        }

        ASSERT(facet_offset->size() == facet_count->size());
        ASSERT(facet_count->size() == mesh.materials().size() + 1);
        ASSERT(sorted_facet_indices->size() == mesh.facets().size());
        ASSERT(sorted_facet_indices->size() == unique_elements(*sorted_facet_indices).size());
        ASSERT(std::is_sorted(std::cbegin(*sorted_facet_indices), std::cend(*sorted_facet_indices), [&](int a, int b) {
                int m_a = material_index(mesh.facets()[a].material);
                int m_b = material_index(mesh.facets()[b].material);
                return m_a < m_b;
        }));
}
