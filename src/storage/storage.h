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

#include "events.h"
#include "object_id.h"
#include "options.h"

#include <src/com/variant.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress_list.h>

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

struct ObjectStorage
{
        // std::variant<std::shared_ptr<const Mesh<MIN, MeshFloat>>, ...,
        //   std::shared_ptr<const Mesh<MAX, MeshFloat>>>
        using MeshVariant = SequenceVariant2ConstType2<
                STORAGE_MIN_DIMENSIONS,
                STORAGE_MAX_DIMENSIONS,
                std::shared_ptr,
                Mesh,
                StorageMeshFloatingPoint>;
        using ObjectVariant =
                SequenceVariant2ConstType2<STORAGE_MIN_DIMENSIONS, STORAGE_MAX_DIMENSIONS, std::shared_ptr, Obj>;

        virtual ~ObjectStorage() = default;

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

        virtual bool manifold_constructor_exists() const = 0;

        virtual bool object_exists(ObjectId id) const = 0;
        virtual ObjectVariant object(ObjectId id) const = 0;

        virtual bool mesh_exists(ObjectId id) const = 0;
        virtual MeshVariant mesh(ObjectId id) const = 0;

        struct FileFormat
        {
                std::string name;
                std::vector<std::string> extensions;
        };
        virtual std::vector<FileFormat> formats_for_save(unsigned dimension) const = 0;
        virtual std::vector<FileFormat> formats_for_load() const = 0;

        virtual void set_object_size_and_position(double size, const vec3& position) = 0;

        virtual void compute_bound_cocone(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                double rho,
                double alpha) = 0;

        virtual void load_from_file(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double rho,
                double alpha) = 0;

        virtual void load_from_repository(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                int dimension,
                const std::string& object_name,
                double rho,
                double alpha,
                int point_count) = 0;

        virtual void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const = 0;
};

std::unique_ptr<ObjectStorage> create_object_storage(
        int mesh_threads,
        const ObjectStorageEvents& event_emitter,
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler);
