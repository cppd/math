/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "shape.h"

#include "../objects.h"
#include "../shading/shading.h"

#include <src/geometry/spatial/parallelotope.h>
#include <src/geometry/spatial/shape_intersection.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Parallelotope final : public Shape<N, T>, public Surface<N, T>
{
        geometry::Parallelotope<N, T> m_parallelotope;
        SurfaceProperties<N, T> m_surface_properties;
        T m_metalness;
        T m_roughness;
        Color m_color;
        Shading<N, T> m_shading;

public:
        template <typename... V>
        Parallelotope(
                T metalness,
                T roughness,
                const Color& color,
                Color::DataType alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_parallelotope(org, e...),
                  m_metalness(std::clamp(metalness, T(0), T(1))),
                  m_roughness(std::clamp(roughness, T(0), T(1))),
                  m_color(color.clamped())
        {
                m_surface_properties.set_alpha(alpha);
        }

        void set_light_source(const Color& color)
        {
                m_surface_properties.set_light_source_color(color);
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override
        {
                return m_parallelotope.intersect(r);
        }

        std::optional<Intersection<N, T>> intersect(const Ray<N, T>&, T bounding_distance) const override
        {
                // всегда есть пересечение, так как прошла проверка intersect_bounding

                std::optional<Intersection<N, T>> intersection(std::in_place);
                intersection->distance = bounding_distance;
                intersection->surface = this;
                // задавать значение для intersection->data не нужно, так как это один объект

                return intersection;
        }

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* /*intersection_data*/) const override
        {
                SurfaceProperties<N, T> s = m_surface_properties;

                s.set_geometric_normal(m_parallelotope.normal(p));

                return s;
        }

        Color lighting(
                const Vector<N, T>& /*p*/,
                const void* /*intersection_data*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                return m_shading.direct_lighting(m_metalness, m_roughness, m_color, n, v, l);
        }

        SurfaceReflection<N, T> reflection(
                RandomEngine<T>& random_engine,
                const Vector<N, T>& /*p*/,
                const void* /*intersection_data*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override
        {
                return m_shading.reflection(random_engine, m_metalness, m_roughness, m_color, n, v);
        }

        geometry::BoundingBox<N, T> bounding_box() const override
        {
                return geometry::BoundingBox<N, T>(m_parallelotope.vertices());
        }

        std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>
                intersection_function() const override
        {
                return [w = geometry::ShapeWrapperForIntersection(m_parallelotope)](
                               const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>& p)
                {
                        return geometry::shape_intersection(w, p);
                };
        }
};
}
