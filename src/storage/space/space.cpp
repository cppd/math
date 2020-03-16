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

#include "space.h"

#include <src/com/names.h>
#include <src/com/time.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/graph/mst.h>
#include <src/model/mesh_utility.h>

namespace
{
std::string bound_cocone_text_rho_alpha(double rho, double alpha)
{
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

        std::vector<ConvexHullFacet<N>> convex_hull_facets;

        double start_time = time_in_seconds();

        compute_convex_hull(points, &convex_hull_facets, progress);

        LOG("Convex hull created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        std::vector<std::array<int, N>> facets;
        facets.clear();
        facets.reserve(convex_hull_facets.size());
        for (const ConvexHullFacet<N>& f : convex_hull_facets)
        {
                facets.push_back(f.vertices());
        }

        return mesh::create_mesh_for_facets(points, facets);
}
}

template <size_t N, typename MeshFloat>
ObjectStorageSpace<N, MeshFloat>::ObjectStorageSpace(
        int mesh_threads,
        const ObjectStorageEvents& event_emitter,
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
        : m_mesh_threads(mesh_threads),
          m_event_emitter(event_emitter),
          m_exception_handler(exception_handler),
          m_object_repository(create_object_repository<N>())
{
}

template <size_t N, typename MeshFloat>
template <typename F>
void ObjectStorageSpace<N, MeshFloat>::catch_all(const F& function) const
{
        std::string message;
        try
        {
                function(&message);
        }
        catch (...)
        {
                m_exception_handler(std::current_exception(), message);
        }
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::set_object_size_and_position(double size, const vec3& position)
{
        m_object_size = size;
        m_object_position = position;
}

template <size_t N, typename MeshFloat>
std::vector<std::string> ObjectStorageSpace<N, MeshFloat>::repository_point_object_names() const
{
        return m_object_repository->point_object_names();
}

template <size_t N, typename MeshFloat>
bool ObjectStorageSpace<N, MeshFloat>::object_exists(ObjectId id) const
{
        return m_objects.get(id) != nullptr;
}

template <size_t N, typename MeshFloat>
std::shared_ptr<const MeshObject<N>> ObjectStorageSpace<N, MeshFloat>::object(ObjectId id) const
{
        return m_objects.get(id);
}

template <size_t N, typename MeshFloat>
bool ObjectStorageSpace<N, MeshFloat>::mesh_exists(ObjectId id) const
{
        return m_meshes.get(id) != nullptr;
}

template <size_t N, typename MeshFloat>
std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> ObjectStorageSpace<N, MeshFloat>::mesh(ObjectId id) const
{
        return m_meshes.get(id);
}

template <size_t N, typename MeshFloat>
std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> ObjectStorageSpace<N, MeshFloat>::build_mesh(
        ProgressRatioList* progress_list,
        const MeshObject<N>& object)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (object.mesh().facets.empty())
        {
                return nullptr;
        }
        std::lock_guard lg(m_mesh_sequential_mutex);
        ProgressRatio progress(progress_list);
        return std::make_shared<const SpatialMeshModel<N, MeshFloat>>(
                object.mesh(), to_matrix<MeshFloat>(object.matrix()), m_mesh_threads, &progress);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::add_object_and_mesh(
        ProgressRatioList* progress_list,
        const std::shared_ptr<const MeshObject<N>>& object)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        m_objects.set(object->id(), object);
        m_event_emitter.object_loaded(object->id(), N);

        std::shared_ptr ptr = build_mesh(progress_list, *object);
        m_meshes.set(object->id(), std::move(ptr));
        m_event_emitter.mesh_loaded(object->id());
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::convex_hull(
        ProgressRatioList* progress_list,
        const std::shared_ptr<const MeshObject<N>>& object)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

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

        std::shared_ptr<MeshObject<N>> ch_object =
                std::make_shared<MeshObject<N>>(std::move(ch_mesh), object->matrix(), "Convex Hull");

        add_object_and_mesh(progress_list, ch_object);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::cocone(
        ProgressRatioList* progress_list,
        const ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const MeshObject<N>& object)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

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

        std::shared_ptr<MeshObject<N>> cocone_object =
                std::make_shared<MeshObject<N>>(std::move(cocone_mesh), object.matrix(), "Cocone");

        add_object_and_mesh(progress_list, cocone_object);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::bound_cocone(
        ProgressRatioList* progress_list,
        const ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const MeshObject<N>& object,
        double rho,
        double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

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
        std::shared_ptr<MeshObject<N>> bound_cocone_object =
                std::make_shared<MeshObject<N>>(std::move(bound_cocone_mesh), object.matrix(), name);

        add_object_and_mesh(progress_list, bound_cocone_object);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::build_mst(
        ProgressRatioList* progress_list,
        const ManifoldConstructor<N>& constructor,
        const std::vector<Vector<N, float>>& points,
        const MeshObject<N>& object)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::vector<std::array<int, 2>> mst_lines;
        {
                ProgressRatio progress(progress_list);

                mst_lines = minimum_spanning_tree(points, constructor.delaunay_objects(), &progress);
        }
        std::unique_ptr<const mesh::Mesh<N>> mst_mesh = mesh::create_mesh_for_lines(points, mst_lines);
        if (mst_mesh->lines.empty())
        {
                return;
        }

        std::shared_ptr<MeshObject<N>> mst_object =
                std::make_shared<MeshObject<N>>(std::move(mst_mesh), object.matrix(), "MST");

        add_object_and_mesh(progress_list, mst_object);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::manifold_constructor(
        ProgressRatioList* progress_list,
        const std::unordered_set<ComputationType>& objects,
        const MeshObject<N>& object,
        double rho,
        double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (objects.count(ComputationType::Cocone) == 0 && objects.count(ComputationType::BoundCocone) == 0 &&
            objects.count(ComputationType::Mst) == 0)
        {
                return;
        }

        auto points_iter = m_manifold_constructors_points.find(object.id());
        if (points_iter == m_manifold_constructors_points.cend())
        {
                std::vector<Vector<N, float>> points;
                if (!object.mesh().facets.empty())
                {
                        points = unique_facet_vertices(object.mesh());
                }
                else
                {
                        points = unique_point_vertices(object.mesh());
                }
                points_iter = m_manifold_constructors_points.emplace(object.id(), std::move(points)).first;
        }

        auto constructor_iter = m_manifold_constructors.find(object.id());
        if (constructor_iter == m_manifold_constructors.cend())
        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::unique_ptr<ManifoldConstructor<N>> constructor =
                        create_manifold_constructor(points_iter->second, &progress);
                constructor_iter = m_manifold_constructors.emplace(object.id(), std::move(constructor)).first;

                LOG("Manifold reconstruction first phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        std::vector<std::thread> threads;

        if (objects.count(ComputationType::Cocone) > 0)
        {
                threads.emplace_back([&]() {
                        catch_all([&](std::string* message) {
                                *message = "Cocone reconstruction in " + space_name(N);

                                cocone(progress_list, *constructor_iter->second, points_iter->second, object);
                        });
                });
        }

        if (objects.count(ComputationType::BoundCocone) > 0)
        {
                threads.emplace_back([&]() {
                        catch_all([&](std::string* message) {
                                *message = "BoundCocone reconstruction in " + space_name(N);

                                bound_cocone(
                                        progress_list, *constructor_iter->second, points_iter->second, object, rho,
                                        alpha);
                        });
                });
        }

        if (objects.count(ComputationType::Mst) > 0)
        {
                threads.emplace_back([&]() {
                        catch_all([&](std::string* message) {
                                *message = "Minimum spanning tree in " + space_name(N);

                                build_mst(progress_list, *constructor_iter->second, points_iter->second, object);
                        });
                });
        }

        for (std::thread& thread : threads)
        {
                thread.join();
        }
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::clear_all_data()
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        m_event_emitter.object_deleted_all(N);

        m_manifold_constructors_points.clear();
        m_manifold_constructors.clear();
        m_objects.reset_all();
        m_meshes.reset_all();
}

template <size_t N, typename MeshFloat>
template <typename ObjectLoaded>
void ObjectStorageSpace<N, MeshFloat>::load_object(
        const std::unordered_set<ComputationType>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        std::unique_ptr<const mesh::Mesh<N>>&& mesh,
        double rho,
        double alpha,
        const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (mesh->facets.empty() && mesh->points.empty())
        {
                error("Facets or points not found");
        }

        if (!mesh->facets.empty() && !mesh->points.empty())
        {
                error("Facets and points together in one object are not supported");
        }

        // Вызов object_loaded() предназначен для вызовов
        // clear_all_data() для всех объектов всех измерений
        object_loaded();

        m_event_emitter.file_loaded(object_name, N, objects);

        Matrix<N + 1, N + 1, double> matrix;
        if constexpr (N == 3)
        {
                ASSERT(m_object_size != 0);
                matrix = model_vertex_matrix(*mesh, m_object_size, m_object_position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        const std::shared_ptr<const MeshObject<N>> model_object =
                std::make_shared<const MeshObject<N>>(std::move(mesh), matrix, "Model");

        std::vector<std::thread> threads;

        threads.emplace_back([&]() {
                catch_all([&](std::string* message) {
                        *message = "Model";

                        add_object_and_mesh(progress_list, model_object);
                });
        });

        if (objects.count(ComputationType::ConvexHull) > 0)
        {
                threads.emplace_back([&]() {
                        catch_all([&](std::string* message) {
                                *message = "Convex hull";

                                convex_hull(progress_list, model_object);
                        });
                });
        }

        if (objects.count(ComputationType::Cocone) > 0 || objects.count(ComputationType::BoundCocone) > 0 ||
            objects.count(ComputationType::Mst) > 0)
        {
                threads.emplace_back([&]() {
                        catch_all([&](std::string* message) {
                                *message = "Manifold constructor";

                                manifold_constructor(progress_list, objects, *model_object, rho, alpha);
                        });
                });
        }

        for (std::thread& thread : threads)
        {
                thread.join();
        }
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::compute_bound_cocone(
        ProgressRatioList* progress_list,
        ObjectId id,
        double rho,
        double alpha)
{
        const std::shared_ptr<const MeshObject<N>> obj = m_objects.get(id);
        if (!obj)
        {
                error("No object found to compute BoundCocone");
        }
        manifold_constructor(progress_list, {ComputationType::BoundCocone}, *obj, rho, alpha);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::load_from_file(
        const std::unordered_set<ComputationType>& objects,
        ProgressRatioList* progress_list,
        const std::string& file_name,
        double rho,
        double alpha,
        const std::function<void()>& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::unique_ptr<const mesh::Mesh<N>> mesh;
        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading file: %p%");
                mesh = mesh::load_geometry<N>(file_name, &progress);
        }
        load_object(objects, progress_list, file_name, std::move(mesh), rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::load_from_repository(
        const std::unordered_set<ComputationType>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        double rho,
        double alpha,
        int point_count,
        const std::function<void()>& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::unique_ptr<const mesh::Mesh<N>> mesh;
        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading object: %p%");
                mesh = mesh::create_mesh_for_points(m_object_repository->point_object(object_name, point_count));
        }
        load_object(objects, progress_list, object_name, std::move(mesh), rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::save(ObjectId id, const std::string& file_name, const std::string& name) const
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const MeshObject<N>> object = m_objects.get(id);

        if (!object)
        {
                m_event_emitter.message_warning("No object to export");
                return;
        }

        save_geometry(object->mesh(), file_name, name);
}

//

template class ObjectStorageSpace<3, float>;
template class ObjectStorageSpace<3, double>;
template class ObjectStorageSpace<4, float>;
template class ObjectStorageSpace<4, double>;
template class ObjectStorageSpace<5, float>;
template class ObjectStorageSpace<5, double>;
