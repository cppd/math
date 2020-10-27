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

#include "objects.h"

#include "mesh/mesh_object.h"
#include "space/hyperplane_parallelotope.h"
#include "space/parallelotope.h"
#include "space/parallelotope_algorithm.h"

#include <src/numerical/algorithm.h>

#include <memory>

namespace painter
{
template <size_t N, typename T>
class VisibleHyperplaneParallelotope final : public GenericObject<N, T>, public Surface<N, T>
{
        HyperplaneParallelotope<N, T> m_hyperplane_parallelotope;
        SurfaceProperties<N, T> m_surface_properties;

public:
        template <typename... V>
        VisibleHyperplaneParallelotope(
                const Color& color,
                const Color::DataType& diffuse,
                const Color::DataType& alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_hyperplane_parallelotope(org, e...)
        {
                static_assert((std::is_same_v<V, Vector<N, T>> && ...));

                m_surface_properties.set_color(color);
                m_surface_properties.set_diffuse(diffuse);
                m_surface_properties.set_alpha(alpha);
        }

        void set_light_source(const Color& color)
        {
                m_surface_properties.set_light_source_color(color);
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_hyperplane_parallelotope.intersect(r, t);
        }

        bool intersect_precise(
                const Ray<N, T>&,
                T approximate_t,
                T* t,
                const Surface<N, T>** surface,
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
                SurfaceProperties<N, T> s = m_surface_properties;

                s.set_geometric_normal(m_hyperplane_parallelotope.normal(p));

                return s;
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                min_max_vector(hyperplane_parallelotope_vertices(m_hyperplane_parallelotope), min, max);
        }
};

template <size_t N, typename T>
class VisibleParallelotope final : public GenericObject<N, T>, public Surface<N, T>
{
        Parallelotope<N, T> m_parallelotope;
        SurfaceProperties<N, T> m_surface_properties;

public:
        template <typename... V>
        VisibleParallelotope(
                const Color& color,
                const Color::DataType& diffuse,
                const Color::DataType& alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_parallelotope(org, e...)
        {
                static_assert((std::is_same_v<V, Vector<N, T>> && ...));

                m_surface_properties.set_color(color);
                m_surface_properties.set_diffuse(diffuse);
                m_surface_properties.set_alpha(alpha);
        }

        void set_light_source(const Color& color)
        {
                m_surface_properties.set_light_source_color(color);
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_parallelotope.intersect(r, t);
        }

        bool intersect_precise(
                const Ray<N, T>&,
                T approximate_t,
                T* t,
                const Surface<N, T>** surface,
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
                SurfaceProperties<N, T> s = m_surface_properties;

                s.set_geometric_normal(m_parallelotope.normal(p));

                return s;
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                min_max_vector(m_parallelotope.vertices(), min, max);
        }
};

template <size_t N, typename T>
class VisibleSharedMesh final : public GenericObject<N, T>, public Surface<N, T>
{
        std::shared_ptr<const MeshObject<N, T>> m_mesh;

public:
        explicit VisibleSharedMesh(const std::shared_ptr<const MeshObject<N, T>>& mesh) : m_mesh(mesh)
        {
        }

        bool intersect_approximate(const Ray<N, T>& r, T* t) const override
        {
                return m_mesh->intersect_approximate(r, t);
        }

        bool intersect_precise(
                const Ray<N, T>& ray,
                T approximate_t,
                T* t,
                const Surface<N, T>** surface,
                const void** intersection_data) const override
        {
                if (m_mesh->intersect_precise(ray, approximate_t, t, intersection_data))
                {
                        *surface = this;
                        return true;
                }
                return false;
        }

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* intersection_data) const override
        {
                return m_mesh->surface_properties(p, intersection_data);
        }

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const override
        {
                m_mesh->min_max(min, max);
        }
};
}
