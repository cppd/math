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

#include "load.h"

#include "dimension.h"
#include "mesh.h"

#include <src/com/error.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume_object.h>
#include <src/model/volume_utility.h>
#include <src/numerical/vec.h>

#include <memory>
#include <string>

namespace process
{
namespace
{
template <size_t N>
std::shared_ptr<mesh::MeshObject<N>> create_mesh_object(
        const std::string& name,
        double size,
        const vec3& position,
        std::unique_ptr<mesh::Mesh<N>>&& mesh)
{
        if (mesh->facets.empty() && mesh->points.empty())
        {
                error("Facets or points not found");
        }

        if (!mesh->facets.empty() && !mesh->points.empty())
        {
                error("Facets and points together in one object are not supported");
        }

        Matrix<N + 1, N + 1, double> matrix;
        if constexpr (N == 3)
        {
                ASSERT(size != 0);
                matrix = mesh::model_matrix_for_size_and_position(*mesh, size, position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::shared_ptr<mesh::MeshObject<N>> mesh_object =
                std::make_shared<mesh::MeshObject<N>>(std::move(mesh), matrix, name);

        mesh_object->created();

        return mesh_object;
}

template <size_t N>
std::shared_ptr<volume::VolumeObject<N>> create_volume_object(
        const std::string& name,
        double size,
        const vec3& position,
        std::unique_ptr<volume::Volume<N>>&& volume)
{
        Matrix<N + 1, N + 1, double> matrix;
        if constexpr (N == 3)
        {
                ASSERT(size != 0);
                matrix = volume::model_matrix_for_size_and_position(*volume, size, position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::shared_ptr<volume::VolumeObject<N>> volume_object =
                std::make_shared<volume::VolumeObject<N>>(std::move(volume), matrix, name);

        volume_object->update_all();

        return volume_object;
}
}

void load_from_file(
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        ProgressRatioList* progress_list,
        const std::string& file_name,
        double object_size,
        const vec3& object_position,
        double rho,
        double alpha,
        const std::function<void()>& load_event)
{
        unsigned dimension = mesh::file_dimension(file_name);

        apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                std::unique_ptr<mesh::Mesh<N>> mesh;

                {
                        ProgressRatio progress(progress_list);
                        progress.set_text("Loading file: %p%");
                        mesh = mesh::load<N>(file_name, &progress);
                }

                load_event();

                std::shared_ptr<mesh::MeshObject<N>> mesh_object =
                        create_mesh_object("Model", object_size, object_position, std::move(mesh));

                compute<N>(
                        progress_list, build_convex_hull, build_cocone, build_bound_cocone, build_mst, *mesh_object,
                        rho, alpha);
        });
}

void load_from_mesh_repository(
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        ProgressRatioList* progress_list,
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        double rho,
        double alpha,
        int point_count,
        const std::function<void()>& load_event,
        const storage::Repository& repository)
{
        apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                std::unique_ptr<mesh::Mesh<N>> mesh = repository.mesh<N>(object_name, point_count);

                load_event();

                std::shared_ptr<mesh::MeshObject<N>> mesh_object =
                        create_mesh_object(object_name, object_size, object_position, std::move(mesh));

                compute<N>(
                        progress_list, build_convex_hull, build_cocone, build_bound_cocone, build_mst, *mesh_object,
                        rho, alpha);
        });
}

void load_from_volume_repository(
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        int image_size,
        const storage::Repository& repository)
{
        apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                std::unique_ptr<volume::Volume<N>> volume = repository.volume<N>(object_name, image_size);

                create_volume_object(object_name, object_size, object_position, std::move(volume));
        });
}
}
