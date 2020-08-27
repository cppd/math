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

#include "cornell_box.h"

#include "../visible_lights.h"
#include "../visible_projectors.h"
#include "../visible_shapes.h"

#include <src/color/colors.h>
#include <src/model/mesh_utility.h>

namespace painter
{
namespace
{
template <typename T>
class CornellBoxScene : public PaintObjects<3, T>
{
        std::vector<const GenericObject<3, T>*> m_objects;
        std::vector<const LightSource<3, T>*> m_light_sources;
        std::unique_ptr<VisiblePerspectiveProjector<3, T>> m_perspective_projector;
        std::unique_ptr<VisibleParallelProjector<3, T>> m_parallel_projector;
        std::unique_ptr<VisibleSphericalProjector<3, T>> m_spherical_projector;

        Color m_background_color;
        std::optional<Color> m_background_light_source_color;

        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_rectangle_back;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_rectangle_top;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_rectangle_bottom;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_rectangle_left;
        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_rectangle_right;

        std::unique_ptr<VisibleParallelotope<3, T>> m_box;

        std::unique_ptr<VisibleHyperplaneParallelotope<3, T>> m_lamp;

        std::unique_ptr<VisibleSharedMesh<3, T>> m_mesh;

        std::unique_ptr<VisibleConstantLight<3, T>> m_constant_light;
        std::unique_ptr<VisiblePointLight<3, T>> m_point_light;

        //

        const std::vector<const GenericObject<3, T>*>& objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource<3, T>*>& light_sources() const override
        {
                return m_light_sources;
        }

        const Projector<3, T>& projector() const override
        {
                return *m_perspective_projector;
                // return *m_parallel_projector;
                // return *m_spherical_projector;
        }

        const Color& background_color() const override
        {
                return m_background_color;
        }

        const std::optional<Color>& background_light_source_color() const override
        {
                return m_background_light_source_color;
        }

        //

        void create_scene(
                int width,
                int height,
                T size,
                const Vector<3, T>& camera_direction,
                const Vector<3, T>& camera_up);

public:
        CornellBoxScene(
                int width,
                int height,
                const std::string& obj_file_name,
                T size,
                const Color& default_color,
                const Color::DataType& diffuse,
                const Vector<3, T>& camera_direction,
                const Vector<3, T>& camera_up);

        CornellBoxScene(
                int width,
                int height,
                const std::shared_ptr<const MeshObject<3, T>>& mesh,
                T size,
                const Vector<3, T>& camera_direction,
                const Vector<3, T>& camera_up);
};

template <typename T>
void CornellBoxScene<T>::create_scene(
        int width,
        int height,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        m_objects.push_back(m_mesh.get());

        //

        size *= T(1.5);

        Vector<3, T> dir = camera_direction.normalized();
        Vector<3, T> right = cross(camera_direction, camera_up).normalized();
        Vector<3, T> up = cross(right, dir).normalized();
        Vector<3, T> view_point = T(1.0 / 6) * size * up - size * dir;

        Vector<3, T> lower_left = view_point + T(0.5) * size * (dir - right - up);
        Vector<3, T> lower_right = view_point + T(0.5) * size * (dir + right - up);
        Vector<3, T> upper_left = view_point + T(0.5) * size * (dir - right + up);

        //

        constexpr Color::DataType DIFFUSE = 1;
        constexpr Color::DataType ALPHA = 1;

        m_rectangle_back = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left + size * dir, size * right, size * up);

        m_rectangle_top = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, upper_left, size * dir, size * right);

        m_rectangle_bottom = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left, size * dir, size * right);

        m_rectangle_left = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::RED, DIFFUSE, ALPHA, lower_left, size * dir, size * up);

        m_rectangle_right = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::GREEN, DIFFUSE, ALPHA, lower_right, size * dir, size * up);

        //

        const std::array<int, 2> screen_sizes{width, height};
        const std::array<Vector<3, T>, 2> screen_axes{right, up};
        m_perspective_projector =
                std::make_unique<VisiblePerspectiveProjector<3, T>>(view_point, dir, screen_axes, 70, screen_sizes);
        m_parallel_projector = std::make_unique<VisibleParallelProjector<3, T>>(
                view_point, dir, screen_axes, size / width, screen_sizes);
        m_spherical_projector =
                std::make_unique<VisibleSphericalProjector<3, T>>(view_point, dir, screen_axes, 80, screen_sizes);

        m_box = std::make_unique<VisibleParallelotope<3, T>>(
                colors::MAGENTA, DIFFUSE, ALPHA, lower_left + size * (T(0.7) * dir + T(0.8) * right + T(0.1) * up),
                T(0.1) * size * right, T(0.8) * size * up, T(0.1) * size * dir);

        Vector<3, T> upper_center = upper_left - T(0.001) * size * up + T(0.5) * size * right + T(0.5) * size * dir;

        m_lamp = std::make_unique<VisibleHyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, upper_center - T(0.1) * size * dir - T(0.1) * size * right,
                T(0.2) * size * right, T(0.2) * size * dir);
        m_lamp->set_light_source(Color(50));

        m_constant_light = std::make_unique<VisibleConstantLight<3, T>>(upper_center, Color(1));
        m_point_light = std::make_unique<VisiblePointLight<3, T>>(upper_center, Color(1), 1);

        m_background_color = colors::BLACK;

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

template <typename T>
CornellBoxScene<T>::CornellBoxScene(
        int width,
        int height,
        const std::string& obj_file_name,
        T size,
        const Color& default_color,
        const Color::DataType& diffuse,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        ProgressRatio progress(nullptr);

        std::shared_ptr<const MeshObject<3, T>> painter_mesh;
        {
                std::unique_ptr<const mesh::Mesh<3>> mesh = mesh::load<3>(obj_file_name, &progress);
                mat4 vertex_matrix = model_matrix_for_size_and_position(*mesh, size, vec3(0));
                mesh::MeshObject<3> mesh_object(std::move(mesh), vertex_matrix, "");
                {
                        mesh::Writing writing(&mesh_object);
                        writing.set_color(default_color);
                        writing.set_diffuse(diffuse);
                }
                {
                        std::vector<const mesh::MeshObject<3>*> meshes;
                        meshes.emplace_back(&mesh_object);
                        painter_mesh = std::make_shared<const MeshObject<3, T>>(meshes, &progress);
                }
        }

        m_mesh = std::make_unique<VisibleSharedMesh<3, T>>(painter_mesh);

        create_scene(width, height, size, camera_direction, camera_up);
}

template <typename T>
CornellBoxScene<T>::CornellBoxScene(
        int width,
        int height,
        const std::shared_ptr<const MeshObject<3, T>>& mesh,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        m_mesh = std::make_unique<VisibleSharedMesh<3, T>>(mesh);

        create_scene(width, height, size, camera_direction, camera_up);
}
}

template <typename T>
std::unique_ptr<const PaintObjects<3, T>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        T size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        return std::make_unique<CornellBoxScene<T>>(
                width, height, obj_file_name, size, default_color, diffuse, camera_direction, camera_up);
}

template <typename T>
std::unique_ptr<const PaintObjects<3, T>> cornell_box_scene(
        int width,
        int height,
        const std::shared_ptr<const MeshObject<3, T>>& mesh,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        return std::make_unique<CornellBoxScene<T>>(width, height, mesh, size, camera_direction, camera_up);
}

//

template std::unique_ptr<const PaintObjects<3, float>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        float size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, float>& camera_direction,
        const Vector<3, float>& camera_up);

template std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        double size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, double>& camera_direction,
        const Vector<3, double>& camera_up);

template std::unique_ptr<const PaintObjects<3, float>> cornell_box_scene(
        int width,
        int height,
        const std::shared_ptr<const MeshObject<3, float>>& mesh,
        float size,
        const Vector<3, float>& camera_direction,
        const Vector<3, float>& camera_up);

template std::unique_ptr<const PaintObjects<3, double>> cornell_box_scene(
        int width,
        int height,
        const std::shared_ptr<const MeshObject<3, double>>& mesh,
        double size,
        const Vector<3, double>& camera_direction,
        const Vector<3, double>& camera_up);
}
