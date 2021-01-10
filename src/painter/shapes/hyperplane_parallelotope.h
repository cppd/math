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

#include "shading.h"
#include "shape.h"

#include "../objects.h"
#include "../space/hyperplane_parallelotope.h"
#include "../space/shape_intersection.h"

namespace ns::painter::shapes
{
template <std::size_t N, typename T>
class HyperplaneParallelotope final : public Shape<N, T>, public Surface<N, T>
{
        painter::HyperplaneParallelotope<N, T> m_hyperplane_parallelotope;
        SurfaceProperties<N, T> m_surface_properties;
        Color m_color;
        T m_diffuse;
        T m_specular;
        T m_specular_power;

public:
        template <typename... V>
        HyperplaneParallelotope(
                const Color& color,
                Color::DataType metalness,
                Color::DataType specular_power,
                Color::DataType alpha,
                const Vector<N, T>& org,
                const V&... e)
                : m_hyperplane_parallelotope(org, e...)
        {
                m_color = color;

                std::tie(m_diffuse, m_specular, m_specular_power) =
                        prepare_shading_parameters(metalness, specular_power);

                m_surface_properties.set_alpha(alpha);
        }

        void set_light_source(const Color& color)
        {
                m_surface_properties.set_light_source_color(color);
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override
        {
                return m_hyperplane_parallelotope.intersect(r);
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

                s.set_geometric_normal(m_hyperplane_parallelotope.normal(p));

                return s;
        }

        Color direct_lighting(
                const Vector<N, T>& /*p*/,
                const void* /*intersection_data*/,
                const Vector<N, T>& shading_normal,
                const Vector<N, T>& dir_reflection,
                const Vector<N, T>& dir_to_light) const override
        {
                return surface_lighting(
                        dir_to_light, shading_normal, dir_reflection, m_color, m_diffuse, m_specular, m_specular_power);
        }

        SurfaceReflection<N, T> reflection(
                const Vector<N, T>& /*p*/,
                const void* /*intersection_data*/,
                const Vector<N, T>& shading_normal,
                const Vector<N, T>& dir_reflection,
                RandomEngine<T>& random_engine) const override
        {
                return surface_ray_direction(
                        shading_normal, dir_reflection, m_color, m_diffuse, m_specular_power, random_engine);
        }

        BoundingBox<N, T> bounding_box() const override
        {
                return BoundingBox<N, T>(m_hyperplane_parallelotope.vertices());
        }

        std::function<bool(const ShapeWrapperForIntersection<painter::ParallelotopeAA<N, T>>&)> intersection_function()
                const override
        {
                return [w = ShapeWrapperForIntersection(m_hyperplane_parallelotope)](
                               const ShapeWrapperForIntersection<painter::ParallelotopeAA<N, T>>& p)
                {
                        return shape_intersection(w, p);
                };
        }
};
}
