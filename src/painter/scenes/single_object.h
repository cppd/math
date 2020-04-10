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

#include "../objects.h"
#include "../shapes/mesh.h"
#include "../visible_lights.h"
#include "../visible_projectors.h"
#include "../visible_shapes.h"

#include <src/color/color.h>
#include <src/com/type/limit.h>

#include <memory>

namespace painter
{
namespace single_object_scene_implementation
{
template <size_t N, typename T>
class SingleObjectScene final : public PaintObjects<N, T>
{
        VisibleSharedMesh<N, T> m_object;
        std::unique_ptr<const Projector<N, T>> m_projector;
        std::unique_ptr<const LightSource<N, T>> m_light_source;
        SurfaceProperties<N, T> m_default_surface_properties;

        std::vector<const GenericObject<N, T>*> m_objects;
        std::vector<const LightSource<N, T>*> m_light_sources;

        //

        const std::vector<const GenericObject<N, T>*>& objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource<N, T>*>& light_sources() const override
        {
                return m_light_sources;
        }

        const Projector<N, T>& projector() const override
        {
                return *m_projector;
        }

        const SurfaceProperties<N, T>& default_surface_properties() const override
        {
                return m_default_surface_properties;
        }

public:
        SingleObjectScene(
                const Color& background_color,
                const Color& default_color,
                Color::DataType diffuse,
                std::unique_ptr<const Projector<N, T>>&& projector,
                std::unique_ptr<const LightSource<N, T>>&& light_source,
                std::shared_ptr<const MeshObject<N, T>>&& mesh)
                : m_object(std::move(mesh)), m_projector(std::move(projector)), m_light_source(std::move(light_source))
        {
                m_default_surface_properties.set_color(background_color);
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(true);
                m_default_surface_properties.set_light_source_color(Color(background_color.luminance()));

                m_object.set_color(default_color);
                m_object.set_diffuse_and_fresnel(diffuse, 0);
                m_object.set_light_source(false);

                m_light_sources.push_back(m_light_source.get());

                m_objects.push_back(&m_object);
        }
};
}

template <size_t N, typename T>
std::unique_ptr<const PaintObjects<N, T>> single_object_scene(
        const Color& background_color,
        const Color& default_color,
        Color::DataType diffuse,
        std::unique_ptr<const Projector<N, T>>&& projector,
        std::unique_ptr<const LightSource<N, T>>&& light_source,
        std::shared_ptr<const MeshObject<N, T>> mesh)
{
        ASSERT(projector && light_source && mesh);

        namespace impl = single_object_scene_implementation;

        return std::make_unique<impl::SingleObjectScene<N, T>>(
                background_color, default_color, diffuse, std::move(projector), std::move(light_source),
                std::move(mesh));
}

template <size_t N, typename T>
std::unique_ptr<const PaintObjects<N, T>> single_object_scene(
        const Color& background_color,
        const Color& default_color,
        Color::DataType diffuse,
        int min_screen_size,
        int max_screen_size,
        std::shared_ptr<const MeshObject<N, T>> mesh)
{
        LOG("Creating single object scene...");

        ASSERT(mesh);

        if (min_screen_size < 3)
        {
                error("Min screen size (" + to_string(min_screen_size) + ") is too small");
        }

        if (min_screen_size > max_screen_size)
        {
                error("Wrong min and max screen sizes: min = " + to_string(min_screen_size) +
                      ", max = " + to_string(max_screen_size));
        }

        Vector<N, T> min;
        Vector<N, T> max;
        mesh->min_max(&min, &max);
        Vector<N, T> object_size = max - min;
        Vector<N, T> center = min + object_size / T(2);

        //

        T max_projected_object_size = limits<T>::lowest();
        for (unsigned i = 0; i < N - 1; ++i) // кроме последнего измерения, по которому находится камера
        {
                max_projected_object_size = std::max(object_size[i], max_projected_object_size);
        }
        if (max_projected_object_size == 0)
        {
                error("Object is a point on the screen");
        }

        std::array<int, N - 1> screen_size;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                int size_in_pixels = std::lround((object_size[i] / max_projected_object_size) * max_screen_size);
                screen_size[i] = std::clamp(size_in_pixels, min_screen_size, max_screen_size);
        }

        Vector<N, T> camera_position(center);
        camera_position[N - 1] = max[N - 1] + object_size.norm();

        Vector<N, T> camera_direction(0);
        camera_direction[N - 1] = -1;

        std::array<Vector<N, T>, N - 1> screen_axes;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        screen_axes[i][n] = (i != n) ? 0 : 1;
                }
        }

        T units_per_pixel = max_projected_object_size / max_screen_size;

        std::unique_ptr<const Projector<N, T>> projector = std::make_unique<const VisibleParallelProjector<N, T>>(
                camera_position, camera_direction, screen_axes, units_per_pixel, screen_size);

        //

        Vector<N, T> light_position(max + T(100) * (max - center));

        std::unique_ptr<const LightSource<N, T>> light_source =
                std::make_unique<const VisibleConstantLight<N, T>>(light_position, Color(1));

        //

        namespace impl = single_object_scene_implementation;

        return std::make_unique<impl::SingleObjectScene<N, T>>(
                background_color, default_color, diffuse, std::move(projector), std::move(light_source),
                std::move(mesh));
}
}
