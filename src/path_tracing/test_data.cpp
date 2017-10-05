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

#include "test_data.h"

#include "path_tracing/light_source.h"
#include "path_tracing/projector.h"
#include "path_tracing/visible_shapes.h"

constexpr int PIXEL_RESOLUTION = 3;

namespace
{
class CornellBox : public PaintObjects
{
        std::vector<const GenericObject*> m_objects;
        std::vector<const LightSource*> m_light_sources;
        std::unique_ptr<PerspectiveProjector> m_perspective_projector;
        std::unique_ptr<ParallelProjector> m_parallel_projector;
        std::unique_ptr<SphericalProjector> m_spherical_projector;
        SurfaceProperties m_default_surface_properties;

        std::unique_ptr<VisibleRectangle> m_rectangle_back;
        std::unique_ptr<VisibleRectangle> m_rectangle_top;
        std::unique_ptr<VisibleRectangle> m_rectangle_bottom;
        std::unique_ptr<VisibleRectangle> m_rectangle_left;
        std::unique_ptr<VisibleRectangle> m_rectangle_right;

        std::unique_ptr<VisibleParallelepiped> m_box1, m_box2;

        std::unique_ptr<VisibleRectangle> m_lamp;

        std::unique_ptr<ConstantLight> m_constant_light;
        std::unique_ptr<PointLight> m_point_light;

public:
        CornellBox(int width, int height)
        {
                m_perspective_projector = std::make_unique<PerspectiveProjector>(vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), 60,
                                                                                 width, height, PIXEL_RESOLUTION);

                m_parallel_projector = std::make_unique<ParallelProjector>(vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), 2, width,
                                                                           height, PIXEL_RESOLUTION);

                m_spherical_projector = std::make_unique<SphericalProjector>(vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), 80,
                                                                             width, height, PIXEL_RESOLUTION);

                //

                m_default_surface_properties.set_color(vec3(0.0, 0.0, 0.0));
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(false);
                m_default_surface_properties.set_light_source_color(vec3(0, 0, 0));

                m_rectangle_back = std::make_unique<VisibleRectangle>(vec3(-1, -0.5, -0.5), vec3(0, 0, 1), vec3(0, 1, 0));
                m_rectangle_back->set_color(vec3(1, 1, 1));
                m_rectangle_back->set_diffuse_and_fresnel(1, 0);
                m_rectangle_back->set_light_source(false);

                m_rectangle_top = std::make_unique<VisibleRectangle>(vec3(1, -0.5, 0.5), vec3(-2, 0, 0), vec3(0, 1, 0));
                m_rectangle_top->set_color(vec3(1, 1, 1));
                m_rectangle_top->set_diffuse_and_fresnel(1, 0);
                m_rectangle_top->set_light_source(false);

                m_rectangle_bottom = std::make_unique<VisibleRectangle>(vec3(1, -0.5, -0.5), vec3(-2, 0, 0), vec3(0, 1, 0));
                m_rectangle_bottom->set_color(vec3(1, 1, 1));
                m_rectangle_bottom->set_diffuse_and_fresnel(1, 0);
                m_rectangle_bottom->set_light_source(false);

                m_rectangle_left = std::make_unique<VisibleRectangle>(vec3(1, -0.5, 0.5), vec3(-2, 0, 0), vec3(0, 0, -1));
                m_rectangle_left->set_color(vec3(1, 0, 0));
                m_rectangle_left->set_diffuse_and_fresnel(1, 0);
                m_rectangle_left->set_light_source(false);

                m_rectangle_right = std::make_unique<VisibleRectangle>(vec3(1, 0.5, 0.5), vec3(-2, 0, 0), vec3(0, 0, -1));
                m_rectangle_right->set_color(vec3(0, 1, 0));
                m_rectangle_right->set_diffuse_and_fresnel(1, 0);
                m_rectangle_right->set_light_source(false);

                m_box1 = std::make_unique<VisibleParallelepiped>(vec3(-0.7, 0.2, -0.4), vec3(0.2, 0, 0), vec3(0, 0.2, 0),
                                                                 vec3(0, 0, 0.2));
                m_box1->set_color(vec3(1, 1, 0));
                m_box1->set_diffuse_and_fresnel(1, 0);
                m_box1->set_light_source(false);

                m_box2 = std::make_unique<VisibleParallelepiped>(vec3(-0.4, -0.4, -0.3), vec3(0.2, 0, 0), vec3(0, 0.2, 0),
                                                                 vec3(0, 0, 0.5));
                m_box2->set_color(vec3(1, 0, 1));
                m_box2->set_diffuse_and_fresnel(1, 0);
                m_box2->set_light_source(false);

                m_lamp = std::make_unique<VisibleRectangle>(vec3(-0.4, -0.1, 0.499), vec3(-0.2, 0, 0), vec3(0, 0.2, 0));
                m_lamp->set_color(vec3(1, 1, 1));
                m_lamp->set_diffuse_and_fresnel(1, 0);
                m_lamp->set_light_source(true);
                m_lamp->set_light_source_color(vec3(50, 50, 50));

                m_constant_light = std::make_unique<ConstantLight>(vec3(1, 0, 0), vec3(1, 1, 1));
                m_point_light = std::make_unique<PointLight>(vec3(-0.5, 0, 0.499), vec3(1, 1, 1), 1);

                m_objects.push_back(m_lamp.get());
                // m_light_sources.push_back(m_constant_light.get());
                // m_light_sources.push_back(m_point_light.get());

                m_objects.push_back(m_rectangle_back.get());
                m_objects.push_back(m_rectangle_top.get());
                m_objects.push_back(m_rectangle_bottom.get());
                m_objects.push_back(m_rectangle_left.get());
                m_objects.push_back(m_rectangle_right.get());

                m_objects.push_back(m_box1.get());
                m_objects.push_back(m_box2.get());
        }

        const std::vector<const GenericObject*>& get_objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource*>& get_light_sources() const override
        {
                return m_light_sources;
        }

        const Projector& get_projector() const override
        {
                return *m_perspective_projector;
                // return *m_parallel_projector;
                // return *m_spherical_projector;
        }

        const SurfaceProperties& get_default_surface_properties() const override
        {
                return m_default_surface_properties;
        }
};
}

std::unique_ptr<const PaintObjects> cornell_box(int width, int height)
{
        return std::make_unique<CornellBox>(width, height);
}
