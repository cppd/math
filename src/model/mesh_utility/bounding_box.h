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
#include <vector>

namespace mesh
{
template <size_t N>
struct BoundingBox
{
        Vector<N, float> min;
        Vector<N, float> max;
};

namespace bounding_box_implementation
{
template <size_t N, typename T>
void init_min_max(Vector<N, T>* min, Vector<N, T>* max)
{
        *min = Vector<N, T>(limits<T>::max());
        *max = Vector<N, T>(limits<T>::lowest());
}

template <size_t N, typename T>
bool min_max_found(const Vector<N, T>& min, const Vector<N, T>& max)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!is_finite(min[i]))
                {
                        error("Mesh min is not finite");
                }
                if (!is_finite(max[i]))
                {
                        error("Mesh max is not finite");
                }
                if (min[i] > max[i])
                {
                        return false;
                }
        }
        return true;
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box_for_vector(const std::vector<std::optional<BoundingBox<N>>>& boxes)
{
        Vector<N, float> min;
        Vector<N, float> max;

        init_min_max(&min, &max);

        for (const std::optional<BoundingBox<N>>& box : boxes)
        {
                if (box)
                {
                        min = min_vector(min, box->min);
                        max = max_vector(max, box->max);
                }
        }

        if (!min_max_found(min, max))
        {
                return std::nullopt;
        }

        BoundingBox<N> b;
        b.min = min;
        b.max = max;
        return b;
}
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box_by_facets(const Mesh<N>& mesh)
{
        namespace impl = bounding_box_implementation;

        if (mesh.facets.empty())
        {
                return std::nullopt;
        }

        Vector<N, float> min;
        Vector<N, float> max;

        impl::init_min_max(&min, &max);

        const int vertex_count = mesh.vertices.size();

        for (const typename Mesh<N>::Facet& face : mesh.facets)
        {
                static_assert(!std::remove_reference_t<decltype(face.vertices)>().empty());
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

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        BoundingBox<N> b;
        b.min = min;
        b.max = max;
        return b;
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box_by_lines(const Mesh<N>& mesh)
{
        namespace impl = bounding_box_implementation;

        if (mesh.lines.empty())
        {
                return std::nullopt;
        }

        Vector<N, float> min;
        Vector<N, float> max;

        impl::init_min_max(&min, &max);

        const int vertex_count = mesh.vertices.size();

        for (const typename Mesh<N>::Line& line : mesh.lines)
        {
                static_assert(!std::remove_reference_t<decltype(line.vertices)>().empty());
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

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        BoundingBox<N> b;
        b.min = min;
        b.max = max;
        return b;
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box_by_points(const Mesh<N>& mesh)
{
        namespace impl = bounding_box_implementation;

        if (mesh.points.empty())
        {
                return std::nullopt;
        }

        Vector<N, float> min;
        Vector<N, float> max;

        impl::init_min_max(&min, &max);

        const int vertex_count = mesh.vertices.size();

        for (const typename Mesh<N>::Point& point : mesh.points)
        {
                int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = min_vector(min, mesh.vertices[index]);
                max = max_vector(max, mesh.vertices[index]);
        }

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        BoundingBox<N> b;
        b.min = min;
        b.max = max;
        return b;
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box_by_facets_and_lines(const Mesh<N>& mesh)
{
        std::vector<std::optional<BoundingBox<N>>> boxes;

        boxes.push_back(bounding_box_by_facets(mesh));
        boxes.push_back(bounding_box_by_lines(mesh));

        return bounding_box_implementation::bounding_box_for_vector(boxes);
}

template <size_t N>
std::optional<BoundingBox<N>> bounding_box(const Mesh<N>& mesh)
{
        std::vector<std::optional<BoundingBox<N>>> boxes;

        boxes.push_back(bounding_box_by_facets(mesh));
        boxes.push_back(bounding_box_by_lines(mesh));
        boxes.push_back(bounding_box_by_points(mesh));

        return bounding_box_implementation::bounding_box_for_vector(boxes);
}
}
