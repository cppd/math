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

#include <optional>
#include <tuple>

namespace mesh_model_size_implementation
{
template <size_t N, typename T>
void initial_min_max(Vector<N, T>* min, Vector<N, T>* max)
{
        *min = Vector<N, T>(limits<T>::max());
        *max = Vector<N, T>(limits<T>::lowest());
}
}

template <size_t N>
std::optional<std::tuple<Vector<N, float>, Vector<N, float>>> mesh_min_max_facets(const MeshModel<N>& mesh)
{
        const int vertex_count = mesh.vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        mesh_model_size_implementation::initial_min_max(&min, &max);

        Vector<N, float> min_check = min;
        Vector<N, float> max_check = max;

        for (const typename MeshModel<N>::Facet& face : mesh.facets)
        {
                for (int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        min = min_vector(min, mesh.vertices[index]);
                        max = max_vector(max, mesh.vertices[index]);
                }
        }

        if (min == min_check || max == max_check)
        {
                return std::nullopt;
        }

        return std::make_tuple(min, max);
}

template <size_t N>
std::optional<std::tuple<Vector<N, float>, Vector<N, float>>> mesh_min_max_lines(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        mesh_model_size_implementation::initial_min_max(&min, &max);

        Vector<N, float> min_check = min;
        Vector<N, float> max_check = max;

        for (const typename MeshModel<N>::Line& line : mesh.lines)
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        min = min_vector(min, mesh.vertices[index]);
                        max = max_vector(max, mesh.vertices[index]);
                }
        }

        if (min == min_check || max == max_check)
        {
                return std::nullopt;
        }

        return std::make_tuple(min, max);
}

template <size_t N>
std::optional<std::tuple<Vector<N, float>, Vector<N, float>>> mesh_min_max_points(const MeshModel<N>& mesh)
{
        int vertex_count = mesh.vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        mesh_model_size_implementation::initial_min_max(&min, &max);

        Vector<N, float> min_check = min;
        Vector<N, float> max_check = max;

        for (const typename MeshModel<N>::Point& point : mesh.points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, mesh.vertices[index]);
                max = max_vector(max, mesh.vertices[index]);
        }

        if (min == min_check || max == max_check)
        {
                return std::nullopt;
        }

        return std::make_tuple(min, max);
}

template <size_t N>
std::tuple<Vector<N, float>, Vector<N, float>> mesh_min_max_facets_lines(const MeshModel<N>& mesh)
{
        std::vector<std::optional<std::tuple<Vector<N, float>, Vector<N, float>>>> sizes;

        sizes.push_back(mesh_min_max_facets(mesh));
        sizes.push_back(mesh_min_max_lines(mesh));

        Vector<N, float> min = Vector<N, float>(limits<float>::max());
        Vector<N, float> max = Vector<N, float>(limits<float>::lowest());
        bool found = false;
        for (const std::optional<std::tuple<Vector<N, float>, Vector<N, float>>>& size : sizes)
        {
                if (size)
                {
                        min = min_vector(min, std::get<0>(*size));
                        max = max_vector(max, std::get<1>(*size));
                        found = true;
                }
        }
        if (!found)
        {
                error("Mesh has neither facets nor lines");
        }

        return std::make_tuple(min, max);
}
