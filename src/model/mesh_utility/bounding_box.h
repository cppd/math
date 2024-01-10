/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::model::mesh
{
template <std::size_t N>
struct BoundingBox final
{
        Vector<N, float> min;
        Vector<N, float> max;
};

namespace bounding_box_implementation
{
template <std::size_t N, typename T>
void init_min_max(Vector<N, T>* const min, Vector<N, T>* const max)
{
        *min = Vector<N, T>(Limits<T>::max());
        *max = Vector<N, T>(Limits<T>::lowest());
}

template <std::size_t N, typename T>
bool min_max_found(const Vector<N, T>& min, const Vector<N, T>& max)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!std::isfinite(min[i]))
                {
                        error("Mesh min is not finite");
                }
                if (!std::isfinite(max[i]))
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

template <std::size_t N>
std::optional<BoundingBox<N>> bounding_box_for_vector(const std::vector<std::optional<BoundingBox<N>>>& boxes)
{
        Vector<N, float> min;
        Vector<N, float> max;

        init_min_max(&min, &max);

        for (const std::optional<BoundingBox<N>>& box : boxes)
        {
                if (box)
                {
                        min = ::ns::min(min, box->min);
                        max = ::ns::max(max, box->max);
                }
        }

        if (!min_max_found(min, max))
        {
                return std::nullopt;
        }

        return {
                {.min = min, .max = max}
        };
}
}

template <std::size_t N>
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
                static_assert(0 < std::tuple_size_v<decltype(face.vertices)>);
                for (const int index : face.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Facet vertex index out of bounds");
                        }

                        min = ::ns::min(min, mesh.vertices[index]);
                        max = ::ns::max(max, mesh.vertices[index]);
                }
        }

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        return {
                {.min = min, .max = max}
        };
}

template <std::size_t N>
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
                static_assert(0 < std::tuple_size_v<decltype(line.vertices)>);
                for (const int index : line.vertices)
                {
                        if (index < 0 || index >= vertex_count)
                        {
                                error("Line vertex index out of bounds");
                        }

                        min = ::ns::min(min, mesh.vertices[index]);
                        max = ::ns::max(max, mesh.vertices[index]);
                }
        }

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        return {
                {.min = min, .max = max}
        };
}

template <std::size_t N>
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
                const int index = point.vertex;

                if (index < 0 || index >= vertex_count)
                {
                        error("Point vertex index out of bounds");
                }

                min = ::ns::min(min, mesh.vertices[index]);
                max = ::ns::max(max, mesh.vertices[index]);
        }

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        return {
                {.min = min, .max = max}
        };
}

template <std::size_t N>
std::optional<BoundingBox<N>> bounding_box_by_facets_and_lines(const Mesh<N>& mesh)
{
        std::vector<std::optional<BoundingBox<N>>> boxes;

        boxes.push_back(bounding_box_by_facets(mesh));
        boxes.push_back(bounding_box_by_lines(mesh));

        return bounding_box_implementation::bounding_box_for_vector(boxes);
}

template <std::size_t N>
std::optional<BoundingBox<N>> bounding_box(const Mesh<N>& mesh)
{
        std::vector<std::optional<BoundingBox<N>>> boxes;

        boxes.push_back(bounding_box_by_facets(mesh));
        boxes.push_back(bounding_box_by_lines(mesh));
        boxes.push_back(bounding_box_by_points(mesh));

        return bounding_box_implementation::bounding_box_for_vector(boxes);
}
}
