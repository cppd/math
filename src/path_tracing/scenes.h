/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/shapes/mesh.h"
#include "path_tracing/visible_shapes.h"

#include <memory>

template <size_t N, typename T>
class OneObject final : public PaintObjects<N, T>
{
        VisibleSharedMesh<N, T> m_object;
        std::unique_ptr<const Projector<N, T>> m_projector;
        std::unique_ptr<const LightSource<N, T>> m_light_source;
        SurfaceProperties<N, T> m_default_surface_properties;

        std::vector<const GenericObject<N, T>*> m_objects;
        std::vector<const LightSource<N, T>*> m_light_sources;

public:
        OneObject(const Color& background_color, const Color& default_color, T diffuse,
                  std::unique_ptr<const Projector<N, T>>&& projector, std::unique_ptr<const LightSource<N, T>>&& light_source,
                  const std::shared_ptr<const Mesh<N, T>>& mesh)
                : m_object(mesh), m_projector(std::move(projector)), m_light_source(std::move(light_source))
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
};

template <size_t N, typename T>
std::unique_ptr<const PaintObjects<N, T>> one_object_scene(const Color& background_color, const Color& default_color, T diffuse,
                                                           std::unique_ptr<const Projector<N, T>>&& projector,
                                                           std::unique_ptr<const LightSource<N, T>>&& light_source,
                                                           const std::shared_ptr<const Mesh<N, T>>& mesh)
{
        return std::make_unique<OneObject<N, T>>(background_color, default_color, diffuse, std::move(projector),
                                                 std::move(light_source), mesh);
}

std::unique_ptr<const PaintObjects<3, double>> cornell_box(int width, int height, const std::string& obj_file_name, double size,
                                                           const Color& default_color, double diffuse,
                                                           const vec3& camera_direction, const vec3& camera_up);

std::unique_ptr<const PaintObjects<3, double>> cornell_box(int width, int height,
                                                           const std::shared_ptr<const Mesh<3, double>>& mesh, double size,
                                                           const Color& default_color, double diffuse,
                                                           const vec3& camera_direction, const vec3& camera_up);
