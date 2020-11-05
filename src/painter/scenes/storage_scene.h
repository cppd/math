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

#include "functions.h"

#include "../objects.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/com/error.h>

#include <memory>

namespace painter
{
template <size_t N, typename T>
class StorageScene final : public Scene<N, T>
{
        std::unique_ptr<const Shape<N, T>> m_shape;
        std::unique_ptr<const Projector<N, T>> m_projector;
        std::unique_ptr<const LightSource<N, T>> m_light_source;

        Color m_background_color;
        Color m_background_light_source_color;

        std::vector<const Shape<N, T>*> m_shapes;
        std::vector<const LightSource<N, T>*> m_light_sources;

        T m_size;

        //

        T size() const override
        {
                return m_size;
        }

        bool intersect(const Ray<N, T>& ray, T* distance, const Surface<N, T>** surface, const void** intersection_data)
                const override
        {
                return ray_intersect(m_shapes, ray, distance, surface, intersection_data);
        }

        bool has_intersection(const Ray<N, T>& ray, const T& distance) const override
        {
                return ray_has_intersection(m_shapes, ray, distance);
        }

        const std::vector<const LightSource<N, T>*>& light_sources() const override
        {
                return m_light_sources;
        }

        const Projector<N, T>& projector() const override
        {
                return *m_projector;
        }

        const Color& background_color() const override
        {
                return m_background_color;
        }

        const Color& background_light_source_color() const override
        {
                return m_background_light_source_color;
        }

public:
        StorageScene(
                const Color& background_color,
                std::unique_ptr<const Projector<N, T>>&& projector,
                std::unique_ptr<const LightSource<N, T>>&& light_source,
                std::unique_ptr<const Shape<N, T>>&& shape)
                : m_shape(std::move(shape)),
                  m_projector(std::move(projector)),
                  m_light_source(std::move(light_source)),
                  m_background_color(background_color)
        {
                ASSERT(m_projector && m_light_source && m_shape);

                m_background_light_source_color = Color(background_color.luminance());

                m_light_sources.push_back(m_light_source.get());
                m_shapes.push_back(m_shape.get());

                m_size = scene_size(m_shapes);
        }
};
}
