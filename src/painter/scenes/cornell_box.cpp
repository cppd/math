/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "cornell_box.h"

#include "com/color/colors.h"
#include "obj/alg/alg.h"
#include "obj/file/file_load.h"
#include "painter/visible_lights.h"
#include "painter/visible_projectors.h"
#include "painter/visible_shapes.h"

namespace
{
class CornellBoxScene : public PaintObjects<3, double>
{
        std::vector<const GenericObject<3, double>*> m_objects;
        std::vector<const LightSource<3, double>*> m_light_sources;
        std::unique_ptr<VisiblePerspectiveProjector<3, double>> m_perspective_projector;
        std::unique_ptr<VisibleParallelProjector<3, double>> m_parallel_projector;
        std::unique_ptr<VisibleSphericalProjector<3, double>> m_spherical_projector;
        SurfaceProperties<3, double> m_default_surface_properties;

        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_rectangle_back;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_rectangle_top;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_rectangle_bottom;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_rectangle_left;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_rectangle_right;

        std::unique_ptr<VisibleParallelotope<3, double>> m_box;

        std::unique_ptr<VisibleHyperplaneParallelotope<3, double>> m_lamp;

        std::unique_ptr<VisibleSharedMesh<3, double>> m_mesh;

        std::unique_ptr<VisibleConstantLight<3, double>> m_constant_light;
        std::unique_ptr<VisiblePointLight<3, double>> m_point_light;

        //

        const std::vector<const GenericObject<3, double>*>& objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource<3, double>*>& light_sources() const override
        {
                return m_light_sources;
        }

        const Projector<3, double>& projector() const override
        {
                return *m_perspective_projector;
                // return *m_parallel_projector;
                // return *m_spherical_projector;
        }

        const SurfaceProperties<3, double>& default_surface_properties() const override
        {
                return m_default_surface_properties;
        }

        //

        void create_scene(int width, int height, double size, const Color& default_color, double diffuse,
                          const vec3& camera_direction, const vec3& camera_up);

public:
        CornellBoxScene(int width, int height, const std::string& obj_file_name, double size, const Color& default_color,
                        double diffuse, const vec3& camera_direction, const vec3& camera_up);

        CornellBoxScene(int width, int height, const std::shared_ptr<const Mesh<3, double>>& mesh, double size,
                        const Color& default_color, double diffuse, const vec3& camera_direction, const vec3& camera_up);
};

CornellBoxScene::CornellBoxScene(int width, int height, const std::string& obj_file_name, double size, const Color& default_color,
                                 double diffuse, const vec3& camera_direction, const vec3& camera_up)
{
        ProgressRatio progress(nullptr);

        std::unique_ptr<Obj<3>> obj = load_obj_from_file<3>(obj_file_name, &progress);

        mat4 vertex_matrix = model_vertex_matrix(*obj, size, vec3(0));

        std::shared_ptr mesh = std::make_shared<Mesh<3, double>>(obj.get(), vertex_matrix, hardware_concurrency(), &progress);

        m_mesh = std::make_unique<VisibleSharedMesh<3, double>>(mesh);

        create_scene(width, height, size, default_color, diffuse, camera_direction, camera_up);
}

CornellBoxScene::CornellBoxScene(int width, int height, const std::shared_ptr<const Mesh<3, double>>& mesh, double size,
                                 const Color& default_color, double diffuse, const vec3& camera_direction, const vec3& camera_up)
{
        m_mesh = std::make_unique<VisibleSharedMesh<3, double>>(mesh);

        create_scene(width, height, size, default_color, diffuse, camera_direction, camera_up);
}

void CornellBoxScene::create_scene(int width, int height, double size, const Color& default_color, double diffuse,
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

        m_rectangle_back =
                std::make_unique<VisibleHyperplaneParallelotope<3, double>>(lower_left + size * dir, size * right, size * up);
        m_rectangle_back->set_color(colors::WHITE);
        m_rectangle_back->set_diffuse_and_fresnel(1, 0);
        m_rectangle_back->set_light_source(false);

        m_rectangle_top = std::make_unique<VisibleHyperplaneParallelotope<3, double>>(upper_left, size * dir, size * right);
        m_rectangle_top->set_color(colors::WHITE);
        m_rectangle_top->set_diffuse_and_fresnel(1, 0);
        m_rectangle_top->set_light_source(false);

        m_rectangle_bottom = std::make_unique<VisibleHyperplaneParallelotope<3, double>>(lower_left, size * dir, size * right);
        m_rectangle_bottom->set_color(colors::WHITE);
        m_rectangle_bottom->set_diffuse_and_fresnel(1, 0);
        m_rectangle_bottom->set_light_source(false);

        m_rectangle_left = std::make_unique<VisibleHyperplaneParallelotope<3, double>>(lower_left, size * dir, size * up);
        m_rectangle_left->set_color(colors::RED);
        m_rectangle_left->set_diffuse_and_fresnel(1, 0);
        m_rectangle_left->set_light_source(false);

        m_rectangle_right = std::make_unique<VisibleHyperplaneParallelotope<3, double>>(lower_right, size * dir, size * up);
        m_rectangle_right->set_color(colors::GREEN);
        m_rectangle_right->set_diffuse_and_fresnel(1, 0);
        m_rectangle_right->set_light_source(false);

        //

        const std::array<int, 2> screen_sizes{width, height};
        const std::array<vec3, 2> screen_axes{right, up};
        m_perspective_projector =
                std::make_unique<VisiblePerspectiveProjector<3, double>>(view_point, dir, screen_axes, 70, screen_sizes);
        m_parallel_projector =
                std::make_unique<VisibleParallelProjector<3, double>>(view_point, dir, screen_axes, size / width, screen_sizes);
        m_spherical_projector =
                std::make_unique<VisibleSphericalProjector<3, double>>(view_point, dir, screen_axes, 80, screen_sizes);

        m_default_surface_properties.set_color(colors::BLACK);
        m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
        m_default_surface_properties.set_light_source(false);
        m_default_surface_properties.set_light_source_color(colors::BLACK);

        m_box = std::make_unique<VisibleParallelotope<3, double>>(lower_left + size * (0.7 * dir + 0.8 * right + 0.1 * up),
                                                                  0.1 * size * right, 0.8 * size * up, 0.1 * size * dir);

        m_box->set_color(colors::MAGENTA);
        m_box->set_diffuse_and_fresnel(1, 0);
        m_box->set_light_source(false);

        vec3 upper_center = upper_left - 0.001 * size * up + 0.5 * size * right + 0.5 * size * dir;

        m_lamp = std::make_unique<VisibleHyperplaneParallelotope<3, double>>(upper_center - 0.1 * size * dir - 0.1 * size * right,
                                                                             0.2 * size * right, 0.2 * size * dir);
        m_lamp->set_color(colors::WHITE);
        m_lamp->set_diffuse_and_fresnel(1, 0);
        m_lamp->set_light_source(true);
        m_lamp->set_light_source_color(Color(50));

        m_constant_light = std::make_unique<VisibleConstantLight<3, double>>(upper_center, Color(1));
        m_point_light = std::make_unique<VisiblePointLight<3, double>>(upper_center, Color(1), 1);

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

}

std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(int width, int height, const std::string& obj_file_name,
                                                                 double size, const Color& default_color, double diffuse,
                                                                 const vec3& camera_direction, const vec3& camera_up)
{
        return std::make_unique<CornellBoxScene>(width, height, obj_file_name, size, default_color, diffuse, camera_direction,
                                                 camera_up);
}

std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(int width, int height,
                                                                 const std::shared_ptr<const Mesh<3, double>>& mesh, double size,
                                                                 const Color& default_color, double diffuse,
                                                                 const vec3& camera_direction, const vec3& camera_up)
{
        return std::make_unique<CornellBoxScene>(width, height, mesh, size, default_color, diffuse, camera_direction, camera_up);
}
