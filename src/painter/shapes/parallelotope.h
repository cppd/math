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
        const geometry::Parallelotope<N, T> m_parallelotope;
        std::optional<Color> m_light_source;
        const T m_metalness;
        const T m_roughness;
        const Color m_color;
        const Color::DataType m_alpha;
        const bool m_alpha_nonzero = m_alpha > 0;

public:
        template <typename... V>
        Parallelotope(
                const T metalness,
                const T roughness,
                const Color& color,
                const Color::DataType alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_parallelotope(org, e...),
                  m_metalness(std::clamp(metalness, T(0), T(1))),
                  m_roughness(std::clamp(roughness, T(0), T(1))),
                  m_color(color.clamped()),
                  m_alpha(std::clamp<Color::DataType>(alpha, 0, 1))
        {
        }

        void set_light_source(const Color& color)
        {
                m_light_source = color;
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override
        {
                if (m_alpha_nonzero || m_light_source)
                {
                        return m_parallelotope.intersect(r);
                }
                return std::nullopt;
        }

        std::optional<Intersection<N, T>> intersect(const Ray<N, T>& ray, const T bounding_distance) const override
        {
                // всегда есть пересечение, так как прошла проверка intersect_bounding

                std::optional<Intersection<N, T>> intersection(std::in_place);
                intersection->point = ray.point(bounding_distance);
                intersection->surface = this;
                // задавать значение для intersection->data не нужно, так как это один объект

                return intersection;
        }

        Vector<N, T> geometric_normal(const Vector<N, T>& point, const void* /*data*/) const override
        {
                return m_parallelotope.normal(point);
        }

        std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& /*point*/, const void* /*data*/) const override
        {
                return std::nullopt;
        }

        std::optional<Color> light_source(const Vector<N, T>& /*point*/, const void* /*data*/) const override
        {
                return m_light_source;
        }

        Color shade(
                const Vector<N, T>& /*point*/,
                const void* /*data*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                return ::ns::painter::shade(m_alpha, m_metalness, m_roughness, m_color, n, v, l);
        }

        ShadeSample<N, T> sample_shade(
                const Vector<N, T>& /*point*/,
                const void* /*data*/,
                RandomEngine<T>& random_engine,
                const ShadeType shade_type,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override
        {
                return ::ns::painter::sample_shade(
                        random_engine, shade_type, m_alpha, m_metalness, m_roughness, m_color, n, v);
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
