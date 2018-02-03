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

#include "scenes.h"

#include "com/color/colors.h"
#include "obj/obj_alg.h"
#include "obj/obj_file_load.h"
#include "path_tracing/lights/light_source.h"
#include "path_tracing/projectors/projector.h"
#include "path_tracing/samplers/sampler.h"
#include "path_tracing/visible_shapes.h"

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

        std::unique_ptr<VisibleParallelepiped> m_box;

        std::unique_ptr<VisibleRectangle> m_lamp;

        std::unique_ptr<VisibleSharedMesh> m_mesh;

        std::unique_ptr<ConstantLight> m_constant_light;
        std::unique_ptr<PointLight> m_point_light;

public:
        CornellBox(int width, int height, const std::string& obj_file_name, double size, const vec3& default_color,
                   double diffuse, const vec3& camera_direction, const vec3& camera_up)
        {
                ProgressRatio progress(nullptr);

                std::unique_ptr<IObj> obj_file = load_obj_from_file(obj_file_name, &progress);

                mat4 vertex_matrix = model_vertex_matrix(obj_file.get(), size, vec3(0));

                std::shared_ptr mesh =
                        std::make_shared<Mesh>(obj_file.get(), vertex_matrix, get_hardware_concurrency(), &progress);

                m_mesh = std::make_unique<VisibleSharedMesh>(mesh);

                make_cornell_box(width, height, size, default_color, diffuse, camera_direction, camera_up);
        }

        CornellBox(int width, int height, const std::shared_ptr<const Mesh>& mesh, double size, const vec3& default_color,
                   double diffuse, const vec3& camera_direction, const vec3& camera_up)
        {
                m_mesh = std::make_unique<VisibleSharedMesh>(mesh);

                make_cornell_box(width, height, size, default_color, diffuse, camera_direction, camera_up);
        }

        void make_cornell_box(int width, int height, double size, const vec3& default_color, double diffuse,
                              const vec3& camera_direction, const vec3& camera_up)
        {
                m_mesh->set_color(default_color);
                m_mesh->set_diffuse_and_fresnel(diffuse, 0);
                m_mesh->set_light_source(false);

                m_objects.push_back(m_mesh.get());

                //

                size *= 1.5;

                vec3 right = normalize(cross(camera_direction, camera_up));
                vec3 up = normalize(camera_up);
                vec3 view_point = (1.0 / 6) * size * up - size * normalize(camera_direction);
                vec3 dir = normalize(camera_direction);

                vec3 lower_left = view_point + 0.5 * size * (dir - right - up);
                vec3 lower_right = view_point + 0.5 * size * (dir + right - up);
                vec3 upper_left = view_point + 0.5 * size * (dir - right + up);

                //

                m_rectangle_back = std::make_unique<VisibleRectangle>(lower_left + size * dir, size * right, size * up);
                m_rectangle_back->set_color(vec3(1, 1, 1));
                m_rectangle_back->set_diffuse_and_fresnel(1, 0);
                m_rectangle_back->set_light_source(false);

                m_rectangle_top = std::make_unique<VisibleRectangle>(upper_left, size * dir, size * right);
                m_rectangle_top->set_color(vec3(1, 1, 1));
                m_rectangle_top->set_diffuse_and_fresnel(1, 0);
                m_rectangle_top->set_light_source(false);

                m_rectangle_bottom = std::make_unique<VisibleRectangle>(lower_left, size * dir, size * right);
                m_rectangle_bottom->set_color(vec3(1, 1, 1));
                m_rectangle_bottom->set_diffuse_and_fresnel(1, 0);
                m_rectangle_bottom->set_light_source(false);

                m_rectangle_left = std::make_unique<VisibleRectangle>(lower_left, size * dir, size * up);
                m_rectangle_left->set_color(vec3(1, 0, 0));
                m_rectangle_left->set_diffuse_and_fresnel(1, 0);
                m_rectangle_left->set_light_source(false);

                m_rectangle_right = std::make_unique<VisibleRectangle>(lower_right, size * dir, size * up);
                m_rectangle_right->set_color(vec3(0, 1, 0));
                m_rectangle_right->set_diffuse_and_fresnel(1, 0);
                m_rectangle_right->set_light_source(false);

                //

                m_perspective_projector = std::make_unique<PerspectiveProjector>(view_point, dir, up, 70, width, height);

                m_parallel_projector = std::make_unique<ParallelProjector>(view_point, dir, up, size, width, height);

                m_spherical_projector = std::make_unique<SphericalProjector>(view_point, dir, up, 80, width, height);

                m_default_surface_properties.set_color(vec3(0.0, 0.0, 0.0));
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(false);
                m_default_surface_properties.set_light_source_color(vec3(0, 0, 0));

                m_box = std::make_unique<VisibleParallelepiped>(lower_left + 0.7 * size * dir + 0.8 * size * right +
                                                                        0.1 * size * up,
                                                                0.1 * size * right, 0.8 * size * up, 0.1 * size * dir);

                m_box->set_color(vec3(1, 0, 1));
                m_box->set_diffuse_and_fresnel(1, 0);
                m_box->set_light_source(false);

                vec3 upper_center = upper_left - 0.001 * size * up + 0.5 * size * right + 0.5 * size * dir;

                m_lamp = std::make_unique<VisibleRectangle>(upper_center - 0.1 * size * dir - 0.1 * size * right,
                                                            0.2 * size * right, 0.2 * size * dir);
                m_lamp->set_color(vec3(1, 1, 1));
                m_lamp->set_diffuse_and_fresnel(1, 0);
                m_lamp->set_light_source(true);
                m_lamp->set_light_source_color(vec3(50, 50, 50));

                m_constant_light = std::make_unique<ConstantLight>(upper_center, vec3(1, 1, 1));
                m_point_light = std::make_unique<PointLight>(upper_center, vec3(1, 1, 1), 1);

                m_objects.push_back(m_lamp.get());
                // m_light_sources.push_back(m_constant_light.get());
                // m_light_sources.push_back(m_point_light.get());

                m_objects.push_back(m_rectangle_back.get());
                m_objects.push_back(m_rectangle_top.get());
                m_objects.push_back(m_rectangle_bottom.get());
                m_objects.push_back(m_rectangle_left.get());
                m_objects.push_back(m_rectangle_right.get());

                m_objects.push_back(m_box.get());
        }

        const std::vector<const GenericObject*>& objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource*>& light_sources() const override
        {
                return m_light_sources;
        }

        const Projector& projector() const override
        {
                return *m_perspective_projector;
                // return *m_parallel_projector;
                // return *m_spherical_projector;
        }

        const SurfaceProperties& default_surface_properties() const override
        {
                return m_default_surface_properties;
        }
};

class OneObject final : public PaintObjects
{
        VisibleSharedMesh m_object;
        std::unique_ptr<const Projector> m_projector;
        std::unique_ptr<const LightSource> m_light_source;
        SurfaceProperties m_default_surface_properties;

        std::vector<const GenericObject*> m_objects;
        std::vector<const LightSource*> m_light_sources;

public:
        OneObject(const vec3& background_color, const vec3& default_color, double diffuse,
                  std::unique_ptr<const Projector>&& projector, std::unique_ptr<const LightSource>&& light_source,
                  const std::shared_ptr<const Mesh>& mesh)
                : m_object(mesh), m_projector(std::move(projector)), m_light_source(std::move(light_source))
        {
                m_default_surface_properties.set_color(background_color);
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(true);
                m_default_surface_properties.set_light_source_color(vec3(luminance_of_rgb(background_color)));

                m_object.set_color(default_color);
                m_object.set_diffuse_and_fresnel(diffuse, 0);
                m_object.set_light_source(false);

                m_light_sources.push_back(m_light_source.get());

                m_objects.push_back(&m_object);
        }

        const std::vector<const GenericObject*>& objects() const override
        {
                return m_objects;
        }
        const std::vector<const LightSource*>& light_sources() const override
        {
                return m_light_sources;
        }
        const Projector& projector() const override
        {
                return *m_projector;
        }
        const SurfaceProperties& default_surface_properties() const override
        {
                return m_default_surface_properties;
        }
};
}

std::unique_ptr<const PaintObjects> cornell_box(int width, int height, const std::string& obj_file_name, double size,
                                                const vec3& default_color, double diffuse, const vec3& camera_direction,
                                                const vec3& camera_up)
{
        return std::make_unique<CornellBox>(width, height, obj_file_name, size, default_color, diffuse, camera_direction,
                                            camera_up);
}

std::unique_ptr<const PaintObjects> cornell_box(int width, int height, const std::shared_ptr<const Mesh>& mesh, double size,
                                                const vec3& default_color, double diffuse, const vec3& camera_direction,
                                                const vec3& camera_up)
{
        return std::make_unique<CornellBox>(width, height, mesh, size, default_color, diffuse, camera_direction, camera_up);
}

std::unique_ptr<const PaintObjects> one_object_scene(const vec3& background_color, const vec3& default_color, double diffuse,
                                                     std::unique_ptr<const Projector>&& projector,
                                                     std::unique_ptr<const LightSource>&& light_source,
                                                     const std::shared_ptr<const Mesh>& mesh)
{
        return std::make_unique<OneObject>(background_color, default_color, diffuse, std::move(projector),
                                           std::move(light_source), mesh);
}
