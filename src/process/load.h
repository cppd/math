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

#include "dimension.h"
#include "mesh.h"
#include "options.h"

#include <src/com/error.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume_object.h>
#include <src/model/volume_utility.h>
#include <src/numerical/vec.h>
#include <src/progress/progress_list.h>
#include <src/storage/repository.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace process
{
template <size_t N>
std::shared_ptr<mesh::MeshObject<N>> load_from_file(
        const std::string& object_name,
        ProgressRatioList* progress_list,
        const std::string& file_name)
{
        std::unique_ptr<mesh::Mesh<N>> mesh;

        {
                ProgressRatio progress(progress_list);
                progress.set_text("Loading file: %p%");
                mesh = mesh::load<N>(file_name, &progress);
        }

        std::shared_ptr<mesh::MeshObject<N>> mesh_object = std::make_shared<mesh::MeshObject<N>>(
                std::move(mesh), mesh::model_matrix_for_size_and_position(*mesh, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        mesh_object->insert();

        return mesh_object;
}

template <size_t N>
std::shared_ptr<mesh::MeshObject<N>> load_from_mesh_repository(
        const std::string& object_name,
        int point_count,
        const storage::Repository& repository)
{
        std::unique_ptr<mesh::Mesh<N>> mesh = repository.mesh<N>(object_name, point_count);

        std::shared_ptr<mesh::MeshObject<N>> mesh_object = std::make_shared<mesh::MeshObject<N>>(
                std::move(mesh), mesh::model_matrix_for_size_and_position(*mesh, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        mesh_object->insert();

        return mesh_object;
}

template <size_t N>
std::shared_ptr<volume::VolumeObject<N>> load_from_volume_repository(
        const std::string& object_name,
        int image_size,
        const storage::Repository& repository)
{
        std::unique_ptr<volume::Volume<N>> volume = repository.volume<N>(object_name, image_size);

        std::shared_ptr<volume::VolumeObject<N>> volume_object = std::make_shared<volume::VolumeObject<N>>(
                std::move(volume),
                volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<N, double>), object_name);

        volume_object->insert();

        return volume_object;
}

template <size_t N, typename Image>
std::shared_ptr<volume::VolumeObject<N>> load_from_volume_image(const std::string& object_name, Image&& image)
{
        std::unique_ptr<volume::Volume<N>> volume = std::make_unique<volume::Volume<N>>();

        volume->image = std::forward<Image>(image);
        volume->matrix = Matrix<N + 1, N + 1, double>(1);

        std::shared_ptr<volume::VolumeObject<N>> volume_object = std::make_shared<volume::VolumeObject<N>>(
                std::move(volume),
                volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<N, double>), object_name);

        volume_object->insert();

        return volume_object;
}
}
