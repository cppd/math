/*
Copyright (C) 2017 Topological Manifold

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

#include "com/log.h"
#include "com/print.h"
#include "path_tracing/shapes/mesh.h"
#include "path_tracing/shapes/rectangle.h"
#include "path_tracing/space/parallelotope.h"

#include <memory>

class VisibleRectangle final : public GenericObject, public Surface, public SurfaceProperties
{
        Rectangle m_rectangle;

public:
        VisibleRectangle(const vec3& org, const vec3& e0, const vec3& e1) : m_rectangle(org, e0, e1)
        {
        }

        // Интерфейс GenericObject
        bool intersect_approximate(const ray3& r, double* t) const override
        {
                return m_rectangle.intersect(r, t);
        }
        bool intersect_precise(const ray3&, double approximate_t, double* t, const Surface** surface,
                               const GeometricObject** /*geometric_object*/) const override
        {
                *t = approximate_t;
                *surface = this;

                // задавать значение для *geometric_object не нужно, так как это один объект

                // всегда есть пересечение, так как прошла проверка intersect_approximate
                return true;
        }

        // Интерфейс Surface
        SurfaceProperties properties(const vec3& p, const GeometricObject* /*geometric_object*/) const override
        {
                SurfaceProperties s = *this;

                s.set_geometric_normal(m_rectangle.normal(p));

                return s;
        }
};

class VisibleParallelepiped final : public GenericObject, public Surface, public SurfaceProperties
{
        Parallelotope<3, double> m_parallelepiped;

public:
        VisibleParallelepiped(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2) : m_parallelepiped(org, e0, e1, e2)
        {
        }

        // Интерфейс GenericObject
        bool intersect_approximate(const ray3& r, double* t) const override
        {
                return m_parallelepiped.intersect(r, t);
        }
        bool intersect_precise(const ray3&, double approximate_t, double* t, const Surface** surface,
                               const GeometricObject** /*geometric_object*/) const override
        {
                *t = approximate_t;
                *surface = this;

                // задавать значение для *geometric_object не нужно, так как это один объект

                // всегда есть пересечение, так как прошла проверка intersect_approximate
                return true;
        }

        // Интерфейс Surface
        SurfaceProperties properties(const vec3& p, const GeometricObject* /*geometric_object*/) const override
        {
                SurfaceProperties s = *this;

                s.set_geometric_normal(m_parallelepiped.normal(p));

                return s;
        }
};

class VisibleSharedMesh final : public GenericObject, public Surface, public SurfaceProperties
{
        std::shared_ptr<const Mesh> m_mesh;

public:
        VisibleSharedMesh(const std::shared_ptr<const Mesh>& mesh) : m_mesh(mesh)
        {
        }

        // Интерфейс GenericObject
        bool intersect_approximate(const ray3& r, double* t) const override
        {
                return m_mesh->intersect_approximate(r, t);
        }
        bool intersect_precise(const ray3& ray, double approximate_t, double* t, const Surface** surface,
                               const GeometricObject** geometric_object) const override
        {
                if (m_mesh->intersect_precise(ray, approximate_t, t, geometric_object))
                {
                        *surface = this;
                        return true;
                }
                else
                {
                        return false;
                }
        }

        // Интерфейс Surface
        SurfaceProperties properties(const vec3& p, const GeometricObject* geometric_object) const override
        {
                SurfaceProperties s = *this;

                s.set_geometric_normal(m_mesh->get_geometric_normal(geometric_object));
                s.set_shading_normal(m_mesh->get_shading_normal(p, geometric_object));
                s.set_triangle_mesh(true);

                if (std::optional<vec3> color = m_mesh->get_color(p, geometric_object))
                {
                        s.set_color(color.value());
                }

                return s;
        }
};
