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

namespace mesh
{
namespace position_implementation
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

template <size_t N, typename T>
void initial_min_max(Vector<N, T>* min, Vector<N, T>* max)
{
        *min = Vector<N, T>(limits<T>::max());
        *max = Vector<N, T>(limits<T>::lowest());
}

template <size_t N>
void set_center_and_length_for_facets(Mesh<N>* mesh)
{
        ASSERT(mesh);

        if (mesh->facets.empty())
        {
                error("No facets");
        }

        int vertex_count = mesh->vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        initial_min_max(&min, &max);

        for (const typename Mesh<N>::Facet& facet : mesh->facets)
        {
                for (int index : facet.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        min = min_vector(min, mesh->vertices[index]);
                        max = max_vector(max, mesh->vertices[index]);
                }
        }

        std::tie(mesh->center, mesh->length) = center_and_length_for_min_max(min, max);
}

template <size_t N>
void set_center_and_length_for_lines(Mesh<N>* mesh)
{
        ASSERT(mesh);

        if (mesh->lines.empty())
        {
                error("No lines");
        }

        int vertex_count = mesh->vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        initial_min_max(&min, &max);

        for (const typename Mesh<N>::Line& line : mesh->lines)
        {
                for (int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        min = min_vector(min, mesh->vertices[index]);
                        max = max_vector(max, mesh->vertices[index]);
                }
        }

        std::tie(mesh->center, mesh->length) = center_and_length_for_min_max(min, max);
}

template <size_t N>
void set_center_and_length_for_points(Mesh<N>* mesh)
{
        ASSERT(mesh);

        if (mesh->points.empty())
        {
                error("No points");
        }

        int vertex_count = mesh->vertices.size();

        Vector<N, float> min;
        Vector<N, float> max;

        initial_min_max(&min, &max);

        for (const typename Mesh<N>::Point& point : mesh->points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, mesh->vertices[index]);
                max = max_vector(max, mesh->vertices[index]);
        }

        std::tie(mesh->center, mesh->length) = center_and_length_for_min_max(min, max);
}
}

template <size_t N>
void set_center_and_length(Mesh<N>* mesh)
{
        int c = 0;
        c += !mesh->facets.empty() ? 1 : 0;
        c += !mesh->lines.empty() ? 1 : 0;
        c += !mesh->points.empty() ? 1 : 0;
        if (c > 1)
        {
                error("Only facets, or lines, or points are supported for mesh center and length");
        }

        namespace impl = position_implementation;

        if (!mesh->facets.empty())
        {
                impl::set_center_and_length_for_facets(mesh);
                return;
        }

        if (!mesh->lines.empty())
        {
                impl::set_center_and_length_for_lines(mesh);
                return;
        }

        if (!mesh->points.empty())
        {
                impl::set_center_and_length_for_points(mesh);
                return;
        }
}
}
