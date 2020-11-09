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

#include "shape.h"

#include "../objects.h"
#include "../space/parallelotope.h"

namespace painter::shapes
{
template <size_t N, typename T>
class Parallelotope final : public Shape<N, T>, public Surface<N, T>
{
        painter::Parallelotope<N, T> m_parallelotope;
        SurfaceProperties<N, T> m_surface_properties;

public:
        template <typename... V>
        Parallelotope(
                const Color& color,
                const Color::DataType& diffuse,
                const Color::DataType& alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_parallelotope(org, e...)
        {
                m_surface_properties.set_color(color);
                m_surface_properties.set_diffuse(diffuse);
                m_surface_properties.set_alpha(alpha);
        }

        void set_light_source(const Color& color)
        {
                m_surface_properties.set_light_source_color(color);
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override
        {
                T t;
                if (m_parallelotope.intersect(r, &t))
                {
                        return t;
                }
                return std::nullopt;
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

        BoundingBox<N, T> bounding_box() const override
        {
                return BoundingBox<N, T>(m_parallelotope.vertices());
        }
};
}
