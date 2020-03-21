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

#include "manage.h"

#include "processor.h"

#include <src/com/error.h>
#include <src/model/mesh_utility.h>

void compute_bound_cocone(
        ProgressRatioList* progress_list,
        ObjectId id,
        double rho,
        double alpha,
        int mesh_threads,
        ObjectMultiStorage* storage)
{
        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                if (!v.storage.object(id))
                                {
                                        return false;
                                }

                                processor::compute_bound_cocone(
                                        progress_list, &v.storage, id, rho, alpha, mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("No object found");
        }
}

void save_to_file(ObjectId id, const std::string& file_name, const std::string& name, const ObjectMultiStorage& storage)
{
        bool found = std::apply(
                [&](const auto&... v) {
                        return ([&]() {
                                if (!v.storage.object(id))
                                {
                                        return false;
                                }

                                processor::save(v.storage, id, file_name, name);

                                return true;
                        }() || ...);
                },
                storage.data());

        if (!found)
        {
                error("No object found");
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
        int mesh_threads,
        const std::function<void(size_t dimension)>& load_event,
        ObjectMultiStorage* storage)
{
        unsigned dimension = mesh::file_dimension(file_name);

        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                constexpr unsigned N = std::remove_reference_t<decltype(v)>::N;

                                if (N != dimension)
                                {
                                        return false;
                                }

                                auto mesh = processor::load_from_file<N>(progress_list, file_name);

                                storage->clear_all_data();
                                load_event(dimension);

                                processor::compute(
                                        progress_list, &v.storage, build_convex_hull, build_cocone, build_bound_cocone,
                                        build_mst, std::move(mesh), object_size, object_position, rho, alpha,
                                        mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("Dimension " + to_string(dimension) + " is not supported, supported dimensions [" +
                      to_string(STORAGE_MIN_DIMENSIONS) + "," + to_string(STORAGE_MAX_DIMENSIONS));
        }
}

void load_from_repository(
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
        ObjectMultiStorage* storage)
{
        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                constexpr unsigned N = std::remove_reference_t<decltype(v)>::N;

                                if (N != dimension)
                                {
                                        return false;
                                }

                                auto mesh = processor::load_from_repository(
                                        progress_list, *v.repository, object_name, point_count);

                                storage->clear_all_data();
                                load_event();

                                processor::compute(
                                        progress_list, &v.storage, build_convex_hull, build_cocone, build_bound_cocone,
                                        build_mst, std::move(mesh), object_size, object_position, rho, alpha,
                                        mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("Dimension " + to_string(dimension) + " is not supported, supported dimensions [" +
                      to_string(STORAGE_MIN_DIMENSIONS) + "," + to_string(STORAGE_MAX_DIMENSIONS));
        }
}

std::vector<FileFormat> formats_for_save(unsigned dimension)
{
        std::vector<FileFormat> v(1);

        v[0].name = "OBJ Files";
        v[0].extensions = {mesh::obj_file_extension(dimension)};

        return v;
}

std::vector<FileFormat> formats_for_load()
{
        std::set<unsigned> dimensions;
        for (int n = STORAGE_MIN_DIMENSIONS; n <= STORAGE_MAX_DIMENSIONS; ++n)
        {
                dimensions.insert(n);
        }

        std::vector<FileFormat> v(1);

        v[0].name = "All Supported Formats";
        for (std::string& s : mesh::obj_file_supported_extensions(dimensions))
        {
                v[0].extensions.push_back(std::move(s));
        }
        for (std::string& s : mesh::txt_file_supported_extensions(dimensions))
        {
                v[0].extensions.push_back(std::move(s));
        }

        return v;
}
