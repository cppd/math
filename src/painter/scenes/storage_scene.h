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
        std::vector<std::unique_ptr<const Shape<N, T>>> m_shapes;
        std::vector<std::unique_ptr<const LightSource<N, T>>> m_light_sources;

        std::unique_ptr<const Projector<N, T>> m_projector;

        Color m_background_color;
        Color m_background_light_source_color;

        std::vector<const Shape<N, T>*> m_shape_pointers;
        std::vector<const LightSource<N, T>*> m_light_source_pointers;

        T m_size;

        //

        template <typename P>
        static std::vector<P*> to_pointers(const std::vector<std::unique_ptr<P>>& objects)
        {
                std::vector<P*> result;
                result.reserve(objects.size());
                for (const std::unique_ptr<P>& p : objects)
                {
                        ASSERT(p);
                        result.push_back(p.get());
                }
                return result;
        }

        T size() const override
        {
                return m_size;
        }

        bool intersect(const Ray<N, T>& ray, T* distance, const Surface<N, T>** surface, const void** intersection_data)
                const override
        {
                return ray_intersect(m_shape_pointers, ray, distance, surface, intersection_data);
        }

        bool has_intersection(const Ray<N, T>& ray, const T& distance) const override
        {
                return ray_has_intersection(m_shape_pointers, ray, distance);
        }

        const std::vector<const LightSource<N, T>*>& light_sources() const override
        {
                return m_light_source_pointers;
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
                std::vector<std::unique_ptr<const LightSource<N, T>>>&& light_sources,
                std::vector<std::unique_ptr<const Shape<N, T>>>&& shapes)
                : m_shapes(std::move(shapes)),
                  m_light_sources(std::move(light_sources)),
                  m_projector(std::move(projector)),
                  m_background_color(background_color),
                  m_background_light_source_color(Color(m_background_color.luminance())),
                  m_shape_pointers(to_pointers(m_shapes)),
                  m_light_source_pointers(to_pointers(m_light_sources)),
                  m_size(scene_size(m_shape_pointers))
        {
                ASSERT(m_projector);
        }
};
}
