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

#include "storage_scene.h"

#include "../shapes/hyperplane_parallelotope.h"
#include "../shapes/mesh.h"
#include "../shapes/parallelotope.h"
#include "../visible_lights.h"
#include "../visible_projectors.h"

#include <src/color/colors.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>

namespace painter
{
namespace
{
template <typename T>
std::unique_ptr<const Scene<3, T>> create_scene(
        int width,
        int height,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up,
        std::unique_ptr<const Shape<3, T>>&& shape)
{
        std::vector<std::unique_ptr<const Shape<3, T>>> shapes;
        std::vector<std::unique_ptr<const LightSource<3, T>>> light_sources;

        shapes.push_back(std::move(shape));

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

        // back
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left + size * dir, size * right, size * up));

        // top
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, upper_left, size * dir, size * right));

        // bottom
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::WHITE, DIFFUSE, ALPHA, lower_left, size * dir, size * right));

        // left
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::RED, DIFFUSE, ALPHA, lower_left, size * dir, size * up));

        // right
        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                colors::GREEN, DIFFUSE, ALPHA, lower_right, size * dir, size * up));

        // box
        shapes.push_back(std::make_unique<shapes::Parallelotope<3, T>>(
                colors::MAGENTA, DIFFUSE, ALPHA, lower_left + size * (T(0.7) * dir + T(0.8) * right + T(0.1) * up),
                T(0.1) * size * right, T(0.8) * size * up, T(0.1) * size * dir));

        const Vector<3, T> upper_center =
                upper_left - T(0.001) * size * up + T(0.5) * size * right + T(0.5) * size * dir;

        // lamp
        {
                std::unique_ptr<shapes::HyperplaneParallelotope<3, T>> lamp =
                        std::make_unique<shapes::HyperplaneParallelotope<3, T>>(
                                colors::WHITE, DIFFUSE, ALPHA,
                                upper_center - T(0.1) * size * dir - T(0.1) * size * right, T(0.2) * size * right,
                                T(0.2) * size * dir);
                lamp->set_light_source(Color(50));
                shapes.push_back(std::move(lamp));
        }

        // constant_light = std::make_unique<VisibleConstantLight<3, T>>(upper_center, Color(1));
        // point_light = std::make_unique<VisiblePointLight<3, T>>(upper_center, Color(1), 1);

        //

        const std::array<int, 2> screen_sizes{width, height};
        const std::array<Vector<3, T>, 2> screen_axes{right, up};

        std::unique_ptr<Projector<3, T>> projector =
                std::make_unique<VisiblePerspectiveProjector<3, T>>(view_point, dir, screen_axes, 70, screen_sizes);
        // std::make_unique<VisibleParallelProjector<3, T>>(view_point, dir, screen_axes, size / width, screen_sizes);
        // std::make_unique<VisibleSphericalProjector<3, T>>(view_point, dir, screen_axes, 80, screen_sizes);

        //

        Color background_color = colors::BLACK;

        return std::make_unique<StorageScene<3, T>>(
                background_color, std::move(projector), std::move(light_sources), std::move(shapes));
}
}

template <typename T>
std::unique_ptr<const Scene<3, T>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        T size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        ProgressRatio progress(nullptr);

        std::unique_ptr<const Shape<3, T>> shape;
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
                        shape = std::make_unique<const shapes::Mesh<3, T>>(meshes, &progress);
                }
        }

        return create_scene(width, height, size, camera_direction, camera_up, std::move(shape));
}

template <typename T>
std::unique_ptr<const Scene<3, T>> cornell_box_scene(
        int width,
        int height,
        std::unique_ptr<const Shape<3, T>>&& shape,
        T size,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        return create_scene(width, height, size, camera_direction, camera_up, std::move(shape));
}

//

template std::unique_ptr<const Scene<3, float>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        float size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, float>& camera_direction,
        const Vector<3, float>& camera_up);

template std::unique_ptr<const Scene<3, double>> cornell_box_scene(
        int width,
        int height,
        const std::string& obj_file_name,
        double size,
        const Color& default_color,
        Color::DataType diffuse,
        const Vector<3, double>& camera_direction,
        const Vector<3, double>& camera_up);

template std::unique_ptr<const Scene<3, float>> cornell_box_scene(
        int width,
        int height,
        std::unique_ptr<const Shape<3, float>>&& shape,
        float size,
        const Vector<3, float>& camera_direction,
        const Vector<3, float>& camera_up);

template std::unique_ptr<const Scene<3, double>> cornell_box_scene(
        int width,
        int height,
        std::unique_ptr<const Shape<3, double>>&& shape,
        double size,
        const Vector<3, double>& camera_direction,
        const Vector<3, double>& camera_up);
}
