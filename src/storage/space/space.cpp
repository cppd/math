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
std::shared_ptr<const mesh::Mesh<N>> ObjectStorageSpace<N, MeshFloat>::object(ObjectId id) const
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
bool ObjectStorageSpace<N, MeshFloat>::manifold_constructor_exists() const
{
        return m_manifold_constructor.get() != nullptr;
}

template <size_t N, typename MeshFloat>
std::string ObjectStorageSpace<N, MeshFloat>::object_name(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return "Model";
        case ObjectType::Cocone:
                return "Cocone";
        case ObjectType::BoundCocone:
                return "BoundCocone";
        }
        error_fatal("Unknown object type");
}

template <size_t N, typename MeshFloat>
ObjectId ObjectStorageSpace<N, MeshFloat>::object_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return ObjectId::Model;
        case ObjectType::Cocone:
                return ObjectId::Cocone;
        case ObjectType::BoundCocone:
                return ObjectId::BoundCocone;
        }
        error_fatal("Unknown object type");
}

template <size_t N, typename MeshFloat>
ObjectId ObjectStorageSpace<N, MeshFloat>::convex_hull_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return ObjectId::ModelConvexHull;
        case ObjectType::Cocone:
                return ObjectId::CoconeConvexHull;
        case ObjectType::BoundCocone:
                return ObjectId::BoundCoconeConvexHull;
        }
        error_fatal("Unknown object type");
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::build_mesh(
        ProgressRatioList* progress_list,
        ObjectId id,
        const mesh::Mesh<N>& mesh)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (mesh.facets.empty())
        {
                return;
        }

        std::lock_guard lg(m_mesh_sequential_mutex);

        ProgressRatio progress(progress_list);

        m_meshes.set(
                id, std::make_shared<const SpatialMeshModel<N, MeshFloat>>(
                            &mesh, to_matrix<MeshFloat>(m_model_vertex_matrix), m_mesh_threads, &progress));

        m_event_emitter.mesh_loaded(id);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::add_object_and_build_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const mesh::Mesh<N>>& mesh)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        ObjectId object_id = object_identifier(object_type);

        if (!(object_id == ObjectId::Model || objects.count(object_id) > 0))
        {
                return;
        }

        if (mesh->facets.empty() && !(object_type == ObjectType::Model && !mesh->points.empty()))
        {
                return;
        }

        m_objects.set(object_id, mesh);
        m_event_emitter.object_loaded(object_id, N);

        build_mesh(progress_list, object_id, *mesh);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::add_object_convex_hull_and_build_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const mesh::Mesh<N>>& mesh)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        ObjectId object_id = convex_hull_identifier(object_type);

        if (!(objects.count(object_id) > 0))
        {
                return;
        }

        if (mesh->facets.empty() && !(object_type == ObjectType::Model && !mesh->points.empty()))
        {
                return;
        }

        std::shared_ptr<const mesh::Mesh<N>> mesh_ch;

        {
                ProgressRatio progress(progress_list);
                progress.set_text(object_name(object_type) + " convex hull in " + space_name(N) + ": %v of %m");

                mesh_ch = mesh_convex_hull(*mesh, &progress);
        }

        if (mesh_ch->facets.empty())
        {
                return;
        }

        m_objects.set(object_id, mesh_ch);
        m_event_emitter.object_loaded(object_id, N);

        build_mesh(progress_list, object_id, *mesh_ch);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::object_and_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const mesh::Mesh<N>>& mesh)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::thread thread_mesh([&]() {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object and mesh";

                        add_object_and_build_mesh(objects, progress_list, object_type, mesh);
                });
        });

        std::thread thread_ch([&]() {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object convex hull and mesh";

                        add_object_convex_hull_and_build_mesh(objects, progress_list, object_type, mesh);
                });
        });

        thread_mesh.join();
        thread_ch.join();
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::cocone(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (objects.count(ObjectId::Cocone) == 0 && objects.count(ObjectId::CoconeConvexHull) == 0)
        {
                return;
        }

        if (!m_manifold_constructor)
        {
                error("No manifold constructor");
        }

        std::shared_ptr<const mesh::Mesh<N>> mesh_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->cocone(&normals, &facets, &progress);

                mesh_cocone = mesh::create_mesh_for_facets(m_manifold_points, normals, facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        object_and_mesh(objects, progress_list, ObjectType::Cocone, mesh_cocone);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::bound_cocone(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        double rho,
        double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (objects.count(ObjectId::BoundCocone) == 0 && objects.count(ObjectId::BoundCoconeConvexHull) == 0)
        {
                return;
        }

        if (!m_manifold_constructor)
        {
                error("No manifold constructor");
        }

        std::shared_ptr<const mesh::Mesh<N>> mesh_bound_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                mesh_bound_cocone = mesh::create_mesh_for_facets(m_manifold_points, normals, facets);

                m_bound_cocone_rho = rho;
                m_bound_cocone_alpha = alpha;

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        m_event_emitter.object_deleted(ObjectId::BoundCocone, N);
        m_event_emitter.object_deleted(ObjectId::BoundCoconeConvexHull, N);
        m_meshes.reset(ObjectId::BoundCocone);
        m_meshes.reset(ObjectId::BoundCoconeConvexHull);
        m_objects.reset(ObjectId::BoundCocone);
        m_objects.reset(ObjectId::BoundCoconeConvexHull);

        m_event_emitter.bound_cocone_loaded(rho, alpha);

        object_and_mesh(objects, progress_list, ObjectType::BoundCocone, mesh_bound_cocone);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::build_mst(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (objects.count(ObjectId::ModelMst) == 0)
        {
                return;
        }

        if (!m_manifold_constructor)
        {
                error("No manifold constructor");
        }

        std::vector<std::array<int, 2>> mst_lines;

        {
                ProgressRatio progress(progress_list);

                mst_lines =
                        minimum_spanning_tree(m_manifold_points, m_manifold_constructor->delaunay_objects(), &progress);
        }

        std::shared_ptr<const mesh::Mesh<N>> mst_mesh = mesh::create_mesh_for_lines(m_manifold_points, mst_lines);

        if (mst_mesh->lines.empty())
        {
                return;
        }

        m_objects.set(ObjectId::ModelMst, mst_mesh);
        m_event_emitter.object_loaded(ObjectId::ModelMst, N);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::manifold_constructor(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        double rho,
        double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (objects.count(ObjectId::Cocone) == 0 && objects.count(ObjectId::CoconeConvexHull) == 0 &&
            objects.count(ObjectId::BoundCocone) == 0 && objects.count(ObjectId::BoundCoconeConvexHull) == 0 &&
            objects.count(ObjectId::ModelMst) == 0)
        {
                return;
        }

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                m_manifold_constructor = create_manifold_constructor(m_manifold_points, &progress);

                LOG("Manifold reconstruction first phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        std::thread thread_cocone([&]() {
                catch_all([&](std::string* message) {
                        *message = "Cocone reconstruction in " + space_name(N);

                        cocone(objects, progress_list);
                });
        });

        std::thread thread_bound_cocone([&]() {
                catch_all([&](std::string* message) {
                        *message = "BoundCocone reconstruction in " + space_name(N);

                        bound_cocone(objects, progress_list, rho, alpha);
                });
        });

        std::thread thread_mst([&]() {
                catch_all([&](std::string* message) {
                        *message = "Minimum spanning tree in " + space_name(N);

                        build_mst(objects, progress_list);
                });
        });

        thread_cocone.join();
        thread_bound_cocone.join();
        thread_mst.join();
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::clear_all_data()
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        m_event_emitter.object_deleted_all(N);
        m_manifold_constructor.reset();
        m_meshes.reset_all();
        m_objects.reset_all();
        m_manifold_points.clear();
        m_manifold_points.shrink_to_fit();
}

template <size_t N, typename MeshFloat>
template <typename ObjectLoaded>
void ObjectStorageSpace<N, MeshFloat>::load_object(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        const std::shared_ptr<const mesh::Mesh<N>>& mesh,
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

        m_manifold_points = !mesh->facets.empty() ? unique_facet_vertices(*mesh) : unique_point_vertices(*mesh);

        if constexpr (N == 3)
        {
                ASSERT(m_object_size != 0);
                m_model_vertex_matrix = model_vertex_matrix(*mesh, m_object_size, m_object_position);
        }
        else
        {
                m_model_vertex_matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::thread thread_model([&]() {
                catch_all([&](std::string* message) {
                        *message = "Object and mesh";

                        object_and_mesh(objects, progress_list, ObjectType::Model, mesh);
                });
        });

        std::thread thread_manifold([&]() {
                catch_all([&](std::string* message) {
                        *message = "Manifold constructor";

                        manifold_constructor(objects, progress_list, rho, alpha);
                });
        });

        thread_model.join();
        thread_manifold.join();
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::compute_bound_cocone(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        double rho,
        double alpha)
{
        bound_cocone(objects, progress_list, rho, alpha);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::load_from_file(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& file_name,
        double rho,
        double alpha,
        const std::function<void()>& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const mesh::Mesh<N>> mesh;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading file: %p%");
                mesh = mesh::load_geometry<N>(file_name, &progress);
        }

        load_object(objects, progress_list, file_name, mesh, rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::load_from_repository(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        double rho,
        double alpha,
        int point_count,
        const std::function<void()>& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const mesh::Mesh<N>> mesh;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading object: %p%");
                mesh = mesh::create_mesh_for_points(m_object_repository->point_object(object_name, point_count));
        }

        load_object(objects, progress_list, object_name, mesh, rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
void ObjectStorageSpace<N, MeshFloat>::save(ObjectId id, const std::string& file_name, const std::string& name) const
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const mesh::Mesh<N>> mesh = m_objects.get(id);

        if (!mesh)
        {
                m_event_emitter.message_warning("No object to export");
                return;
        }

        save_geometry(*mesh, file_name, name);
}

//

template class ObjectStorageSpace<3, float>;
template class ObjectStorageSpace<3, double>;
template class ObjectStorageSpace<4, float>;
template class ObjectStorageSpace<4, double>;
template class ObjectStorageSpace<5, float>;
template class ObjectStorageSpace<5, double>;
