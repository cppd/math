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

#include "objects.h"

#include "path_tracing/shapes/mesh.h"
#include "path_tracing/shapes/rectangle.h"
#include "path_tracing/space/parallelotope.h"

#include <memory>

template <size_t N, typename T>
class VisibleRectangle final : public GenericObject<N, T>, public Surface<N, T>, public SurfaceProperties<N, T>
{
        Rectangle<N, T> m_rectangle;

public:
        template <typename... V>
        VisibleRectangle(const Vector<N, T>& org, const V&... e) : m_rectangle(org, e...)
        {
                static_assert((std::is_same_v<V, Vector<N, T>> && ...));
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_rectangle.intersect(r, t);
        }

        bool intersect_precise(const Ray<N, T>&, T approximate_t, T* t, const Surface<N, T>** surface,
                               const void** /*intersection_data*/) const override
        {
                *t = approximate_t;
                *surface = this;

                // задавать значение для *intersection_data не нужно, так как это один объект

                // всегда есть пересечение, так как прошла проверка intersect_approximate
                return true;
        }

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* /*intersection_data*/) const override
        {
                SurfaceProperties<N, T> s = *this;

                s.set_geometric_normal(m_rectangle.normal(p));

                return s;
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                typename RectangleAlgorithm<Rectangle<N, T>>::Vertices vertices = rectangle_vertices(m_rectangle);

                *min = vertices[0];
                *max = vertices[0];
                for (unsigned i = 1; i < vertices.size(); ++i)
                {
                        *min = min_vector(*min, vertices[i]);
                        *max = max_vector(*max, vertices[i]);
                }
        }
};

template <size_t N, typename T>
class VisibleParallelepiped final : public GenericObject<N, T>, public Surface<N, T>, public SurfaceProperties<N, T>
{
        Parallelotope<N, T> m_parallelepiped;

public:
        template <typename... V>
        VisibleParallelepiped(const Vector<N, T>& org, const V&... e) : m_parallelepiped(org, e...)
        {
                static_assert((std::is_same_v<V, Vector<N, T>> && ...));
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_parallelepiped.intersect(r, t);
        }

        bool intersect_precise(const Ray<N, T>&, T approximate_t, T* t, const Surface<N, T>** surface,
                               const void** /*intersection_data*/) const override
        {
                *t = approximate_t;
                *surface = this;

                // задавать значение для *intersection_data не нужно, так как это один объект

                // всегда есть пересечение, так как прошла проверка intersect_approximate
                return true;
        }

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* /*intersection_data*/) const override
        {
                SurfaceProperties<N, T> s = *this;

                s.set_geometric_normal(m_parallelepiped.normal(p));

                return s;
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                typename ParallelotopeAlgorithm<Parallelotope<N, T>>::Vertices vertices =
                        parallelotope_vertices(m_parallelepiped);

                *min = vertices[0];
                *max = vertices[0];
                for (unsigned i = 1; i < vertices.size(); ++i)
                {
                        *min = min_vector(*min, vertices[i]);
                        *max = max_vector(*max, vertices[i]);
                }
        }
};

template <size_t N, typename T>
class VisibleSharedMesh final : public GenericObject<N, T>, public Surface<N, T>, public SurfaceProperties<N, T>
{
        std::shared_ptr<const Mesh<N, T>> m_mesh;

public:
        VisibleSharedMesh(const std::shared_ptr<const Mesh<N, T>>& mesh) : m_mesh(mesh)
        {
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_mesh->intersect_approximate(r, t);
        }

        bool intersect_precise(const Ray<N, T>& ray, T approximate_t, T* t, const Surface<N, T>** surface,
                               const void** intersection_data) const override
        {
                if (m_mesh->intersect_precise(ray, approximate_t, t, intersection_data))
                {
                        *surface = this;
                        return true;
                }
                else
                {
                        return false;
                }
        }

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* intersection_data) const override
        {
                SurfaceProperties<N, T> s = *this;

                s.set_geometric_normal(m_mesh->geometric_normal(intersection_data));
                s.set_shading_normal(m_mesh->shading_normal(p, intersection_data));
                s.set_mesh(true);

                if (std::optional<Color> color = m_mesh->color(p, intersection_data))
                {
                        s.set_color(color.value());
                }

                return s;
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                m_mesh->min_max(min, max);
        }
};
