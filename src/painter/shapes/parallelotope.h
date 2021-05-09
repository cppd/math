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

#include <src/com/memory_arena.h>
#include <src/geometry/spatial/parallelotope.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/shading/ggx_diffuse.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Parallelotope final : public Shape<N, T>
{
        const geometry::Parallelotope<N, T> m_parallelotope;
        std::optional<Color> m_light_source;
        const T m_metalness;
        const T m_roughness;
        const Color m_color;
        const Color::DataType m_alpha;
        const bool m_alpha_nonzero = m_alpha > 0;

        class IntersectionImpl final : public Surface<N, T>
        {
                const Parallelotope* m_obj;

        public:
                IntersectionImpl(const Vector<N, T>& point, const Parallelotope* obj) : Surface<N, T>(point), m_obj(obj)
                {
                }

                Vector<N, T> geometric_normal() const override
                {
                        return m_obj->m_parallelotope.normal(this->point());
                }

                std::optional<Vector<N, T>> shading_normal() const override
                {
                        return std::nullopt;
                }

                std::optional<Color> light_source() const override
                {
                        return m_obj->m_light_source;
                }

                Color brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
                {
                        return shading::GGXDiffuseBRDF<N, T>::f(
                                m_obj->m_metalness, m_obj->m_roughness, m_obj->m_color, n, v, l);
                }

                shading::Sample<N, T> sample_brdf(
                        RandomEngine<T>& random_engine,
                        const Vector<N, T>& n,
                        const Vector<N, T>& v) const override
                {
                        return shading::GGXDiffuseBRDF<N, T>::sample_f(
                                random_engine, m_obj->m_metalness, m_obj->m_roughness, m_obj->m_color, n, v);
                }
        };

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

        const Surface<N, T>* intersect(const Ray<N, T>& ray, const T bounding_distance) const override
        {
                // всегда есть пересечение, так как прошла проверка intersect_bounding
                return make_arena_ptr<IntersectionImpl>(ray.point(bounding_distance), this);
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
