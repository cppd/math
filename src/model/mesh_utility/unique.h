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

#include <src/com/error.h>

#include <unordered_set>
#include <vector>

namespace mesh
{
namespace unique_implementation
{
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
}

template <size_t N>
std::vector<int> unique_facet_indices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Facet& face : mesh.facets)
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

        return unique_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_line_indices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Line& line : mesh.lines)
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

        return unique_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<int> unique_point_indices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<int> vertices(vertex_count);

        for (const typename MeshModel<N>::Point& point : mesh.points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(index);
        }

        return unique_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_facet_vertices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Facet& face : mesh.facets)
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        vertices.insert(mesh.vertices[index]);
                }
        }

        return unique_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_line_vertices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Line& line : mesh.lines)
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        vertices.insert(mesh.vertices[index]);
                }
        }

        return unique_implementation::to_vector(vertices);
}

template <size_t N>
std::vector<Vector<N, float>> unique_point_vertices(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        std::unordered_set<Vector<N, float>> vertices(vertex_count);

        for (const typename MeshModel<N>::Point& point : mesh.points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                vertices.insert(mesh.vertices[index]);
        }

        return unique_implementation::to_vector(vertices);
}
}
