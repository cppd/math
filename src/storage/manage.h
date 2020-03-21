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

#include "storage.h"

#include <src/progress/progress_list.h>

#include <memory>
#include <string>
#include <vector>

struct StorageManage
{
        virtual ~StorageManage() = default;

        struct RepositoryObjects
        {
                int dimension;
                std::vector<std::string> object_names;
                RepositoryObjects(int dimension_, std::vector<std::string>&& object_names_)
                        : dimension(dimension_), object_names(std::move(object_names_))
                {
                }
        };
        virtual std::vector<RepositoryObjects> repository_point_object_names() const = 0;

        virtual std::optional<ObjectVariant> object(ObjectId id) const = 0;
        virtual std::optional<MeshVariant> mesh(ObjectId id) const = 0;

        struct FileFormat
        {
                std::string name;
                std::vector<std::string> extensions;
        };
        virtual std::vector<FileFormat> formats_for_save(unsigned dimension) const = 0;
        virtual std::vector<FileFormat> formats_for_load() const = 0;

        virtual void compute_bound_cocone(
                ProgressRatioList* progress_list,
                ObjectId id,
                double rho,
                double alpha,
                int mesh_threads) = 0;

        virtual void load_from_file(
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
                const std::function<void(size_t dimension)>& load_event) = 0;

        virtual void load_from_repository(
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
                const std::function<void()>& load_event) = 0;

        virtual void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const = 0;
};

std::unique_ptr<StorageManage> create_storage_manage(ObjectMultiStorage::Data& storage);
