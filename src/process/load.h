/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "options.h"

#include <src/image/image.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume.h>
#include <src/model/volume_object.h>
#include <src/model/volume_utility.h>
#include <src/progress/progress.h>
#include <src/progress/progress_list.h>
#include <src/storage/repository.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace ns::process
{
template <std::size_t N, typename Path>
std::shared_ptr<model::mesh::MeshObject<N>> load_mesh(
        const std::string& object_name,
        progress::RatioList* const progress_list,
        const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        std::unique_ptr<model::mesh::Mesh<N>> mesh = [&]
        {
                progress::Ratio progress(progress_list);
                progress.set_text("Loading: %p%");
                return model::mesh::load<N>(path, &progress);
        }();

        auto mesh_object = std::make_shared<model::mesh::MeshObject<N>>(
                std::move(mesh),
                model::mesh::model_matrix_for_size_and_position(*mesh, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        mesh_object->insert();

        return mesh_object;
}

template <std::size_t N>
std::shared_ptr<model::mesh::MeshObject<N>> load_point_mesh(
        const std::string& object_name,
        const int point_count,
        const storage::Repository& repository)
{
        std::unique_ptr<model::mesh::Mesh<N>> mesh = repository.point_mesh<N>(object_name, point_count);

        auto mesh_object = std::make_shared<model::mesh::MeshObject<N>>(
                std::move(mesh),
                model::mesh::model_matrix_for_size_and_position(*mesh, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        mesh_object->insert();

        return mesh_object;
}

template <std::size_t N>
std::shared_ptr<model::mesh::MeshObject<N>> load_facet_mesh(
        const std::string& object_name,
        const int facet_count,
        const storage::Repository& repository)
{
        std::unique_ptr<model::mesh::Mesh<N>> mesh = repository.facet_mesh<N>(object_name, facet_count);

        auto mesh_object = std::make_shared<model::mesh::MeshObject<N>>(
                std::move(mesh),
                model::mesh::model_matrix_for_size_and_position(*mesh, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        mesh_object->insert();

        return mesh_object;
}

template <std::size_t N, typename Path>
std::shared_ptr<model::volume::VolumeObject<N>> load_volume(
        const std::string& object_name,
        progress::RatioList* const progress_list,
        const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        auto volume = std::make_unique<model::volume::Volume<N>>();

        volume->image = [&]
        {
                progress::Ratio progress(progress_list);
                progress.set_text("Loading: %p%");
                return model::volume::load<N>(path, &progress);
        }();

        volume->matrix = model::volume::matrix_for_image_size(volume->image.size);

        auto volume_object = std::make_shared<model::volume::VolumeObject<N>>(
                std::move(volume),
                model::volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        volume_object->insert();

        return volume_object;
}

template <std::size_t N>
std::shared_ptr<model::volume::VolumeObject<N>> load_volume(
        const std::string& object_name,
        const int image_size,
        const storage::Repository& repository)
{
        std::unique_ptr<model::volume::Volume<N>> volume = repository.volume<N>(object_name, image_size);

        auto volume_object = std::make_shared<model::volume::VolumeObject<N>>(
                std::move(volume),
                model::volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        volume_object->insert();

        return volume_object;
}

template <std::size_t N>
std::shared_ptr<model::volume::VolumeObject<N>> load_volume(const std::string& object_name, image::Image<N>&& image)
{
        auto volume = std::make_unique<model::volume::Volume<N>>();

        volume->image = std::move(image);
        volume->matrix = model::volume::matrix_for_image_size(volume->image.size);

        auto volume_object = std::make_shared<model::volume::VolumeObject<N>>(
                std::move(volume),
                model::volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<N, double>),
                object_name);

        volume_object->insert();

        return volume_object;
}
}
