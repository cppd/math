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

#include "processor/mesh.h"
#include "processor/volume.h"

#include <src/com/error.h>
#include <src/model/mesh_utility.h>

void compute_bound_cocone(
        ProgressRatioList* progress_list,
        ObjectId id,
        double rho,
        double alpha,
        int mesh_threads,
        MultiStorage* storage)
{
        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                if (!v.mesh_object(id))
                                {
                                        return false;
                                }

                                processor::compute_bound_cocone(progress_list, &v, id, rho, alpha, mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("No object found");
        }
}

void save_to_obj(ObjectId id, const std::string& file_name, const std::string& comment, const MultiStorage& storage)
{
        bool found = std::apply(
                [&](const auto&... v) {
                        return ([&]() {
                                if (!v.mesh_object(id))
                                {
                                        return false;
                                }

                                processor::save_to_obj(v, id, file_name, comment);

                                return true;
                        }() || ...);
                },
                storage.data());

        if (!found)
        {
                error("No object found");
        }
}

void save_to_stl(
        ObjectId id,
        const std::string& file_name,
        const std::string& comment,
        const MultiStorage& storage,
        bool ascii_format)
{
        bool found = std::apply(
                [&](const auto&... v) {
                        return ([&]() {
                                if (!v.mesh_object(id))
                                {
                                        return false;
                                }

                                processor::save_to_stl(v, id, file_name, comment, ascii_format);

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
        MultiStorage* storage)
{
        unsigned dimension = mesh::file_dimension(file_name);

        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                constexpr unsigned N = std::remove_reference_t<decltype(v)>::DIMENSION;

                                if (N != dimension)
                                {
                                        return false;
                                }

                                auto mesh = processor::load_from_file<N>(progress_list, file_name);

                                storage->clear();
                                load_event(dimension);

                                processor::compute(
                                        progress_list, &v, build_convex_hull, build_cocone, build_bound_cocone,
                                        build_mst, std::move(mesh), object_size, object_position, rho, alpha,
                                        mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("Dimension " + to_string(dimension) + " is not supported, supported dimensions " +
                      to_string(MultiStorage::supported_dimensions()));
        }
}

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
        MultiStorage* storage)
{
        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                constexpr unsigned N = std::remove_reference_t<decltype(v)>::DIMENSION;

                                if (N != dimension)
                                {
                                        return false;
                                }

                                std::unique_ptr<const mesh::Mesh<N>> mesh =
                                        repository.repository<N>().meshes().object(object_name, point_count);

                                storage->clear();
                                load_event();

                                processor::compute(
                                        progress_list, &v, build_convex_hull, build_cocone, build_bound_cocone,
                                        build_mst, std::move(mesh), object_size, object_position, rho, alpha,
                                        mesh_threads);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("Dimension " + to_string(dimension) + " is not supported, supported dimensions " +
                      to_string(MultiStorage::supported_dimensions()));
        }
}

void add_from_volume_repository(
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        int image_size,
        const MultiRepository& repository,
        MultiStorage* storage)
{
        bool found = std::apply(
                [&](auto&... v) {
                        return ([&]() {
                                constexpr unsigned N = std::remove_reference_t<decltype(v)>::DIMENSION;

                                if (N != dimension)
                                {
                                        return false;
                                }

                                std::unique_ptr<const volume::Volume<N>> volume =
                                        repository.repository<N>().volumes().object(object_name, image_size);

                                processor::compute(&v, std::move(volume), object_size, object_position);

                                return true;
                        }() || ...);
                },
                storage->data());

        if (!found)
        {
                error("Dimension " + to_string(dimension) + " is not supported, supported dimensions " +
                      to_string(MultiStorage::supported_dimensions()));
        }
}
