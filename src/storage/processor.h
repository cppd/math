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

#include "repository/points.h"
#include "repository/volumes.h"

#include <src/com/names.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/graph/mst.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume_utility.h>
#include <src/progress/progress_list.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace processor_implementation
{
inline std::mutex global_mesh_sequential_mutex;

template <typename T>
std::string bound_cocone_text_rho_alpha(T rho, T alpha)
{
        static_assert(std::is_floating_point_v<T>);
        std::string text;
        text += reinterpret_cast<const char*>(u8"ρ ") + to_string_fixed(rho, 3);
        text += "; ";
        text += reinterpret_cast<const char*>(u8"α ") + to_string_fixed(alpha, 3);
        return text;
}

template <size_t N>
std::unique_ptr<const mesh::Mesh<N>> mesh_convex_hull(const mesh::Mesh<N>& mesh, ProgressRatio* progress)
{
        std::vector<Vector<N, float>> points;
        if (!mesh.facets.empty())
        {
                points = unique_facet_vertices(mesh);
        }
        else if (!mesh.points.empty())
        {
                points = unique_point_vertices(mesh);
        }
        else
        {
                error("Faces or points not found for computing convex hull object");
        }

        std::vector<geometry::ConvexHullFacet<N>> convex_hull_facets;

        double start_time = time_in_seconds();

        geometry::compute_convex_hull(points, &convex_hull_facets, progress);

        LOG("Convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        std::vector<std::array<int, N>> facets;
        facets.clear();
        facets.reserve(convex_hull_facets.size());
        for (const geometry::ConvexHullFacet<N>& f : convex_hull_facets)
        {
                facets.push_back(f.vertices());
        }

        return mesh::create_mesh_for_facets(points, facets);
}

template <typename MeshFloat, size_t N>
std::shared_ptr<const painter::MeshObject<N, MeshFloat>> build_painter_mesh_object(
        ProgressRatioList* progress_list,
        const mesh::MeshObject<N>& object,
        int mesh_threads)
{
        if (object.mesh().facets.empty())
        {
                return nullptr;
        }

        std::lock_guard lg(global_mesh_sequential_mutex);

        ProgressRatio progress(progress_list);
        return std::make_shared<const painter::MeshObject<N, MeshFloat>>(
                object.mesh(), to_matrix<MeshFloat>(object.matrix()), mesh_threads, &progress);
}

template <size_t N, typename MeshFloat>
void add_meshes(
        ProgressRatioList* progress_list,
        const std::shared_ptr<const mesh::MeshObject<N>>& object,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        storage->set_mesh_object(object);
        std::shared_ptr ptr = build_painter_mesh_object<MeshFloat>(progress_list, *object, mesh_threads);
        storage->set_painter_mesh_object(object->id(), std::move(ptr));
}

template <size_t N, typename MeshFloat>
void convex_hull(
        ProgressRatioList* progress_list,
        const std::shared_ptr<const mesh::MeshObject<N>>& object,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        std::unique_ptr<const mesh::Mesh<N>> ch_mesh;
        {
                ProgressRatio progress(progress_list);
                progress.set_text(object->name() + " convex hull in " + space_name(N) + ": %v of %m");

                ch_mesh = mesh_convex_hull(object->mesh(), &progress);
        }
        if (ch_mesh->facets.empty())
        {
                return;
        }

        const std::shared_ptr<const mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(ch_mesh), object->matrix(), "Convex Hull");

        add_meshes(progress_list, obj, mesh_threads, storage);
}

template <size_t N, typename MeshFloat>
void cocone(
        ProgressRatioList* progress_list,
        const geometry::ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const mesh::MeshObject<N>& object,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        std::unique_ptr<const mesh::Mesh<N>> cocone_mesh;
        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                constructor.cocone(&normals, &facets, &progress);

                cocone_mesh = mesh::create_mesh_for_facets(points, normals, facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }
        if (cocone_mesh->facets.empty())
        {
                return;
        }

        const std::shared_ptr<const mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(cocone_mesh), object.matrix(), "Cocone");

        add_meshes(progress_list, obj, mesh_threads, storage);
}

template <size_t N, typename MeshFloat>
void bound_cocone(
        ProgressRatioList* progress_list,
        const geometry::ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const mesh::MeshObject<N>& object,
        double rho,
        double alpha,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        std::unique_ptr<const mesh::Mesh<N>> bound_cocone_mesh;
        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                constructor.bound_cocone(rho, alpha, &normals, &facets, &progress);

                bound_cocone_mesh = mesh::create_mesh_for_facets(points, normals, facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }
        if (bound_cocone_mesh->facets.empty())
        {
                return;
        }

        std::string name = "Bound Cocone (" + bound_cocone_text_rho_alpha(rho, alpha) + ")";
        const std::shared_ptr<const mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(bound_cocone_mesh), object.matrix(), name);

        add_meshes(progress_list, obj, mesh_threads, storage);
}

template <size_t N, typename MeshFloat>
void mst(
        ProgressRatioList* progress_list,
        const geometry::ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const mesh::MeshObject<N>& object,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        std::vector<std::array<int, 2>> mst_lines;
        {
                ProgressRatio progress(progress_list);

                mst_lines = geometry::minimum_spanning_tree(points, constructor.delaunay_objects(), &progress);
        }
        std::unique_ptr<const mesh::Mesh<N>> mst_mesh = mesh::create_mesh_for_lines(points, mst_lines);
        if (mst_mesh->lines.empty())
        {
                return;
        }

        const std::shared_ptr<const mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(mst_mesh), object.matrix(), "MST");

        add_meshes(progress_list, obj, mesh_threads, storage);
}

template <size_t N, typename MeshFloat>
void manifold_constructor(
        ProgressRatioList* progress_list,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        const mesh::MeshObject<N>& object,
        double rho,
        double alpha,
        int mesh_threads,
        Storage<N, MeshFloat>* storage)
{
        if (!build_cocone && !build_bound_cocone && !build_mst)
        {
                return;
        }

        std::vector<Vector<N, float>> points;

        if (!object.mesh().facets.empty())
        {
                points = unique_facet_vertices(object.mesh());
        }
        else
        {
                points = unique_point_vertices(object.mesh());
        }

        std::shared_ptr<const geometry::ManifoldConstructor<N>> manifold_constructor_ptr =
                storage->manifold_constructor(object.id());
        if (!manifold_constructor_ptr)
        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                manifold_constructor_ptr = geometry::create_manifold_constructor(points, &progress);
                storage->set_manifold_constructor(object.id(), manifold_constructor_ptr);

                LOG("Manifold reconstruction first phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        ThreadsWithCatch threads(3);
        try
        {
                if (build_cocone)
                {
                        threads.add([&]() {
                                cocone(progress_list, *manifold_constructor_ptr, points, object, mesh_threads, storage);
                        });
                }

                if (build_bound_cocone)
                {
                        threads.add([&]() {
                                bound_cocone(
                                        progress_list, *manifold_constructor_ptr, points, object, rho, alpha,
                                        mesh_threads, storage);
                        });
                }

                if (build_mst)
                {
                        threads.add([&]() {
                                mst(progress_list, *manifold_constructor_ptr, points, object, mesh_threads, storage);
                        });
                }
        }
        catch (...)
        {
                threads.join();
                throw;
        }
        threads.join();
}
}

namespace processor
{
template <size_t N, typename MeshFloat>
void compute_bound_cocone(
        ProgressRatioList* progress_list,
        Storage<N, MeshFloat>* storage,
        ObjectId id,
        double rho,
        double alpha,
        int mesh_threads)
{
        namespace impl = processor_implementation;
        const std::shared_ptr<const mesh::MeshObject<N>> obj = storage->mesh_object(id);
        if (!obj)
        {
                error("No object found to compute BoundCocone");
        }
        constexpr bool build_cocone = false;
        constexpr bool build_bound_cocone = true;
        constexpr bool build_mst = false;
        impl::manifold_constructor(
                progress_list, build_cocone, build_bound_cocone, build_mst, *obj, rho, alpha, mesh_threads, storage);
}

template <size_t N, typename MeshFloat>
void compute(
        ProgressRatioList* progress_list,
        Storage<N, MeshFloat>* storage,
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        std::unique_ptr<const mesh::Mesh<N>>&& mesh,
        double object_size,
        const vec3& object_position,
        double rho,
        double alpha,
        int mesh_threads)
{
        namespace impl = processor_implementation;

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
                ASSERT(object_size != 0);
                matrix = model_matrix_for_size_and_position(*mesh, object_size, object_position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        const std::shared_ptr<const mesh::MeshObject<N>> model_object =
                std::make_shared<const mesh::MeshObject<N>>(std::move(mesh), matrix, "Model");

        ThreadsWithCatch threads(3);
        try
        {
                threads.add([&]() { impl::add_meshes(progress_list, model_object, mesh_threads, storage); });

                if (build_convex_hull)
                {
                        threads.add([&]() { impl::convex_hull(progress_list, model_object, mesh_threads, storage); });
                }

                if (build_cocone || build_bound_cocone || build_mst)
                {
                        threads.add([&]() {
                                impl::manifold_constructor(
                                        progress_list, build_cocone, build_bound_cocone, build_mst, *model_object, rho,
                                        alpha, mesh_threads, storage);
                        });
                }
        }
        catch (...)
        {
                threads.join();
                throw;
        }
        threads.join();
}

template <size_t N>
std::unique_ptr<const mesh::Mesh<N>> load_from_file(ProgressRatioList* progress_list, const std::string& file_name)
{
        ProgressRatio progress(progress_list);
        progress.set_text("Loading file: %p%");
        return mesh::load<N>(file_name, &progress);
}

template <size_t N>
std::unique_ptr<const mesh::Mesh<N>> load_mesh_from_point_repository(
        ProgressRatioList* progress_list,
        const PointObjectRepository<N>& repository,
        const std::string& object_name,
        int point_count)
{
        ProgressRatio progress(progress_list);
        progress.set_text("Loading object: %p%");
        return mesh::create_mesh_for_points(repository.object(object_name, point_count));
}

template <size_t N, typename MeshFloat>
void save_to_obj(
        const Storage<N, MeshFloat>& storage,
        ObjectId id,
        const std::string& file_name,
        const std::string& comment)
{
        const std::shared_ptr<const mesh::MeshObject<N>> object = storage.mesh_object(id);

        if (!object)
        {
                error("No object to export");
        }

        mesh::save_to_obj(object->mesh(), file_name, comment);
}

template <size_t N, typename MeshFloat>
void save_to_stl(
        const Storage<N, MeshFloat>& storage,
        ObjectId id,
        const std::string& file_name,
        const std::string& comment,
        bool ascii_format)
{
        const std::shared_ptr<const mesh::MeshObject<N>> object = storage.mesh_object(id);

        if (!object)
        {
                error("No object to export");
        }

        mesh::save_to_stl(object->mesh(), file_name, comment, ascii_format);
}

//

template <size_t N, typename MeshFloat>
void compute(
        Storage<N, MeshFloat>* storage,
        std::unique_ptr<const volume::Volume<N>>&& volume,
        double object_size,
        const vec3& object_position)
{
        Matrix<N + 1, N + 1, double> matrix;
        if constexpr (N == 3)
        {
                ASSERT(object_size != 0);
                matrix = model_matrix_for_size_and_position(*volume, object_size, object_position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::shared_ptr<const volume::VolumeObject<N>> model_object =
                std::make_shared<const volume::VolumeObject<N>>(std::move(volume), matrix, "Volume");

        storage->set_volume_object(std::move(model_object));
}

template <size_t N>
std::unique_ptr<const volume::Volume<N>> load_volume_from_volume_repository(
        ProgressRatioList* progress_list,
        const VolumeObjectRepository<N>& repository,
        const std::string& object_name,
        int image_size)
{
        ProgressRatio progress(progress_list);
        progress.set_text("Loading object: %p%");
        return repository.object(object_name, image_size);
}
}
