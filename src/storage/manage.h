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

#include "multi_repository.h"
#include "multi_storage.h"

#include <src/progress/progress_list.h>

#include <functional>
#include <string>
#include <vector>

namespace storage
{
void compute_bound_cocone(
        ProgressRatioList* progress_list,
        ObjectId id,
        double rho,
        double alpha,
        int mesh_threads,
        MultiStorage* storage,
        const std::tuple<
                std::function<void(mesh::MeshEvent<3>&&)>,
                std::function<void(mesh::MeshEvent<4>&&)>,
                std::function<void(mesh::MeshEvent<5>&&)>>& event_functions);

void save_to_obj(ObjectId id, const std::string& file_name, const std::string& comment, const MultiStorage& storage);
void save_to_stl(
        ObjectId id,
        const std::string& file_name,
        const std::string& comment,
        const MultiStorage& storage,
        bool ascii_format);

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
        int mesh_threads,
        const std::function<void(size_t dimension)>& load_event,
        MultiStorage* storage,
        const std::tuple<
                std::function<void(mesh::MeshEvent<3>&&)>,
                std::function<void(mesh::MeshEvent<4>&&)>,
                std::function<void(mesh::MeshEvent<5>&&)>>& event_functions);

void load_from_point_repository(
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
        int mesh_threads,
        int point_count,
        const std::function<void()>& load_event,
        const MultiRepository& repository,
        MultiStorage* storage,
        const std::tuple<
                std::function<void(mesh::MeshEvent<3>&&)>,
                std::function<void(mesh::MeshEvent<4>&&)>,
                std::function<void(mesh::MeshEvent<5>&&)>>& event_functions);

void add_from_volume_repository(
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        int image_size,
        const MultiRepository& repository,
        MultiStorage* storage,
        const std::tuple<
                std::function<void(volume::VolumeEvent<3>&&)>,
                std::function<void(volume::VolumeEvent<4>&&)>,
                std::function<void(volume::VolumeEvent<5>&&)>>& event_functions);
}
