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

#include "objects.h"

#include "meshes.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/time.h>
#include <src/com/variant.h>
#include <src/geometry/cocone/reconstruction.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/graph/mst.h>
#include <src/geometry/objects/points.h>
#include <src/numerical/matrix.h>
#include <src/obj/alg/alg.h>
#include <src/obj/create/facets.h>
#include <src/obj/create/lines.h>
#include <src/obj/create/points.h>
#include <src/obj/file.h>
#include <src/obj/obj.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress.h>

#include <mutex>
#include <thread>
#include <unordered_map>

int object_id_to_int(ObjectId id)
{
        return static_cast<int>(id);
}

ObjectId int_to_object_id(int int_id)
{
        ObjectId id = static_cast<ObjectId>(int_id);

        switch (id)
        {
        case ObjectId::Model:
        case ObjectId::ModelMst:
        case ObjectId::ModelConvexHull:
        case ObjectId::Cocone:
        case ObjectId::CoconeConvexHull:
        case ObjectId::BoundCocone:
        case ObjectId::BoundCoconeConvexHull:
                return id;
        }

        error_fatal("Wrong ObjectId value " + to_string(int_id));
}

namespace
{
template <size_t N>
std::unique_ptr<const Obj<N>> obj_convex_hull(const Obj<N>& obj, ProgressRatio* progress)
{
        std::vector<Vector<N, float>> points;
        if (!obj.facets().empty())
        {
                points = unique_facet_vertices(&obj);
        }
        else if (!obj.points().empty())
        {
                points = unique_point_vertices(&obj);
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

        return create_obj_for_facets(points, facets);
}
}

template <size_t N, typename MeshFloat>
class ObjectStorageDimension
{
        static_assert(N >= 3);

        enum class ObjectType
        {
                Model,
                Cocone,
                BoundCocone
        };

        const std::thread::id m_thread_id = std::this_thread::get_id();
        const int m_mesh_threads;

        const ObjectStorageCallback& m_event_emitter;
        std::function<void(const std::exception_ptr& ptr, const std::string& msg)> m_exception_handler;

        //

        const std::unique_ptr<ObjectRepository<N>> m_object_repository;
        Meshes<ObjectId, const Mesh<N, MeshFloat>> m_meshes;
        Meshes<ObjectId, const Obj<N>> m_objects;
        std::vector<Vector<N, float>> m_manifold_points;
        std::unique_ptr<ManifoldConstructor<N>> m_manifold_constructor;
        Matrix<N + 1, N + 1, double> m_model_vertex_matrix;

        //

        std::mutex m_mesh_sequential_mutex;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        double m_object_size = 0;
        vec3 m_object_position = vec3(0);

        template <typename F>
        void catch_all(const F& function) const;

        static std::string object_name(ObjectType object_type);
        static ObjectId object_identifier(ObjectType object_type);
        static ObjectId convex_hull_identifier(ObjectType object_type);

        void build_mst(const std::unordered_set<ObjectId>& objects, ProgressRatioList* progress_list);
        void build_mesh(ProgressRatioList* progress_list, ObjectId id, const Obj<N>& obj);
        void add_object_and_build_mesh(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                ObjectType object_type,
                const std::shared_ptr<const Obj<N>>& obj);
        void add_object_convex_hull_and_build_mesh(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                ObjectType object_type,
                const std::shared_ptr<const Obj<N>>& obj);
        void object_and_mesh(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                ObjectType object_type,
                const std::shared_ptr<const Obj<N>>& obj);
        void manifold_constructor(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                double rho,
                double alpha);
        void cocone(const std::unordered_set<ObjectId>& objects, ProgressRatioList* progress_list);
        void bound_cocone(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                double rho,
                double alpha);

        template <typename ObjectLoaded>
        void load_object(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& object_name,
                const std::shared_ptr<const Obj<N>>& obj,
                double rho,
                double alpha,
                const ObjectLoaded& object_loaded);

public:
        ObjectStorageDimension(
                int mesh_threads,
                const ObjectStorageCallback& event_emitter,
                const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler);

        void set_object_size_and_position(double size, const vec3& position);

        void clear_all_data();

        std::vector<std::string> repository_point_object_names() const;

        bool manifold_constructor_exists() const;

        bool object_exists(ObjectId id) const;
        std::shared_ptr<const Obj<N>> object(ObjectId id) const;

        bool mesh_exists(ObjectId id) const;
        std::shared_ptr<const Mesh<N, MeshFloat>> mesh(ObjectId id) const;

        void compute_bound_cocone(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                double rho,
                double alpha);

        template <typename ObjectLoaded>
        void load_from_file(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double rho,
                double alpha,
                const ObjectLoaded& object_loaded);

        template <typename ObjectLoaded>
        void load_from_repository(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& object_name,
                double rho,
                double alpha,
                int point_count,
                const ObjectLoaded& object_loaded);

        void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const;
};

template <size_t N, typename MeshFloat>
ObjectStorageDimension<N, MeshFloat>::ObjectStorageDimension(
        int mesh_threads,
        const ObjectStorageCallback& event_emitter,
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
        : m_mesh_threads(mesh_threads),
          m_event_emitter(event_emitter),
          m_exception_handler(exception_handler),
          m_object_repository(create_object_repository<N>())
{
}

template <size_t N, typename MeshFloat>
template <typename F>
void ObjectStorageDimension<N, MeshFloat>::catch_all(const F& function) const
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
void ObjectStorageDimension<N, MeshFloat>::set_object_size_and_position(double size, const vec3& position)
{
        m_object_size = size;
        m_object_position = position;
}

template <size_t N, typename MeshFloat>
std::vector<std::string> ObjectStorageDimension<N, MeshFloat>::repository_point_object_names() const
{
        return m_object_repository->point_object_names();
}

template <size_t N, typename MeshFloat>
bool ObjectStorageDimension<N, MeshFloat>::object_exists(ObjectId id) const
{
        return m_objects.get(id) != nullptr;
}

template <size_t N, typename MeshFloat>
std::shared_ptr<const Obj<N>> ObjectStorageDimension<N, MeshFloat>::object(ObjectId id) const
{
        return m_objects.get(id);
}

template <size_t N, typename MeshFloat>
bool ObjectStorageDimension<N, MeshFloat>::mesh_exists(ObjectId id) const
{
        return m_meshes.get(id) != nullptr;
}

template <size_t N, typename MeshFloat>
std::shared_ptr<const Mesh<N, MeshFloat>> ObjectStorageDimension<N, MeshFloat>::mesh(ObjectId id) const
{
        return m_meshes.get(id);
}

template <size_t N, typename MeshFloat>
bool ObjectStorageDimension<N, MeshFloat>::manifold_constructor_exists() const
{
        return m_manifold_constructor.get() != nullptr;
}

template <size_t N, typename MeshFloat>
std::string ObjectStorageDimension<N, MeshFloat>::object_name(ObjectType object_type)
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
ObjectId ObjectStorageDimension<N, MeshFloat>::object_identifier(ObjectType object_type)
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
ObjectId ObjectStorageDimension<N, MeshFloat>::convex_hull_identifier(ObjectType object_type)
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
void ObjectStorageDimension<N, MeshFloat>::build_mesh(ProgressRatioList* progress_list, ObjectId id, const Obj<N>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (obj.facets().empty())
        {
                return;
        }

        std::lock_guard lg(m_mesh_sequential_mutex);

        ProgressRatio progress(progress_list);

        m_meshes.set(
                id, std::make_shared<const Mesh<N, MeshFloat>>(
                            &obj, to_matrix<MeshFloat>(m_model_vertex_matrix), m_mesh_threads, &progress));

        m_event_emitter.mesh_loaded(id);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::add_object_and_build_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const Obj<N>>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        ObjectId object_id = object_identifier(object_type);

        if (!(object_id == ObjectId::Model || objects.count(object_id) > 0))
        {
                return;
        }

        if (obj->facets().empty() && !(object_type == ObjectType::Model && !obj->points().empty()))
        {
                return;
        }

        m_objects.set(object_id, obj);
        m_event_emitter.object_loaded(object_id, N);

        build_mesh(progress_list, object_id, *obj);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::add_object_convex_hull_and_build_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const Obj<N>>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        ObjectId object_id = convex_hull_identifier(object_type);

        if (!(objects.count(object_id) > 0))
        {
                return;
        }

        if (obj->facets().empty() && !(object_type == ObjectType::Model && !obj->points().empty()))
        {
                return;
        }

        std::shared_ptr<const Obj<N>> obj_ch;

        {
                ProgressRatio progress(progress_list);
                progress.set_text(object_name(object_type) + " convex hull in " + space_name(N) + ": %v of %m");

                obj_ch = obj_convex_hull(*obj, &progress);
        }

        if (obj_ch->facets().empty())
        {
                return;
        }

        m_objects.set(object_id, obj_ch);
        m_event_emitter.object_loaded(object_id, N);

        build_mesh(progress_list, object_id, *obj_ch);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::object_and_mesh(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        ObjectType object_type,
        const std::shared_ptr<const Obj<N>>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::thread thread_obj([&]() {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object and mesh";

                        add_object_and_build_mesh(objects, progress_list, object_type, obj);
                });
        });

        std::thread thread_ch([&]() {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object convex hull and mesh";

                        add_object_convex_hull_and_build_mesh(objects, progress_list, object_type, obj);
                });
        });

        thread_obj.join();
        thread_ch.join();
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::cocone(
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

        std::shared_ptr<const Obj<N>> obj_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->cocone(&normals, &facets, &progress);

                obj_cocone = create_obj_for_facets(m_manifold_points, normals, facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) +
                    " s");
        }

        object_and_mesh(objects, progress_list, ObjectType::Cocone, obj_cocone);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::bound_cocone(
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

        std::shared_ptr<const Obj<N>> obj_bound_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                obj_bound_cocone = create_obj_for_facets(m_manifold_points, normals, facets);

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

        object_and_mesh(objects, progress_list, ObjectType::BoundCocone, obj_bound_cocone);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::build_mst(
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

        std::shared_ptr<const Obj<N>> mst_obj = create_obj_for_lines(m_manifold_points, mst_lines);

        if (mst_obj->lines().empty())
        {
                return;
        }

        m_objects.set(ObjectId::ModelMst, mst_obj);
        m_event_emitter.object_loaded(ObjectId::ModelMst, N);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::manifold_constructor(
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
void ObjectStorageDimension<N, MeshFloat>::clear_all_data()
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
void ObjectStorageDimension<N, MeshFloat>::load_object(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        const std::shared_ptr<const Obj<N>>& obj,
        double rho,
        double alpha,
        const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (obj->facets().empty() && obj->points().empty())
        {
                error("Facets or points not found");
        }

        if (!obj->facets().empty() && !obj->points().empty())
        {
                error("Facets and points together in one object are not supported");
        }

        // Вызов object_loaded() предназначен для вызовов
        // clear_all_data() для всех объектов всех измерений
        object_loaded();

        m_event_emitter.file_loaded(object_name, N, objects);

        m_manifold_points =
                !obj->facets().empty() ? unique_facet_vertices(obj.get()) : unique_point_vertices(obj.get());

        if constexpr (N == 3)
        {
                ASSERT(m_object_size != 0);
                m_model_vertex_matrix = model_vertex_matrix(*obj, m_object_size, m_object_position);
        }
        else
        {
                m_model_vertex_matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::thread thread_model([&]() {
                catch_all([&](std::string* message) {
                        *message = "Object and mesh";

                        object_and_mesh(objects, progress_list, ObjectType::Model, obj);
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
void ObjectStorageDimension<N, MeshFloat>::compute_bound_cocone(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        double rho,
        double alpha)
{
        bound_cocone(objects, progress_list, rho, alpha);
}

template <size_t N, typename MeshFloat>
template <typename ObjectLoaded>
void ObjectStorageDimension<N, MeshFloat>::load_from_file(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& file_name,
        double rho,
        double alpha,
        const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading file: %p%");
                obj = load_geometry<N>(file_name, &progress);
        }

        load_object(objects, progress_list, file_name, obj, rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
template <typename ObjectLoaded>
void ObjectStorageDimension<N, MeshFloat>::load_from_repository(
        const std::unordered_set<ObjectId>& objects,
        ProgressRatioList* progress_list,
        const std::string& object_name,
        double rho,
        double alpha,
        int point_count,
        const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Loading object: %p%");
                obj = create_obj_for_points(m_object_repository->point_object(object_name, point_count));
        }

        load_object(objects, progress_list, object_name, obj, rho, alpha, object_loaded);
}

template <size_t N, typename MeshFloat>
void ObjectStorageDimension<N, MeshFloat>::save_to_file(
        ObjectId id,
        const std::string& file_name,
        const std::string& name) const
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj = m_objects.get(id);

        if (!obj)
        {
                m_event_emitter.message_warning("No object to export");
                return;
        }

        save_geometry(obj.get(), file_name, name);
}

//

class ObjectStorageImpl final : public ObjectStorage
{
        static constexpr int MIN = MIN_DIMENSION;
        static constexpr int MAX = MAX_DIMENSION;

        static_assert(MIN >= 3 && MIN <= MAX);

        static constexpr size_t COUNT = MAX - MIN + 1;

        //
        // Каждый элемент контейнера это variant с типами <T<MIN, T2...>, T<MIN + 1, T2...>, ...>.
        // По каждому номеру I от MIN до MAX хранится map[I] = T<I, T2...>.
        // Например, имеется тип Type и числа MIN = 3, MAX = 5.
        // Тип данных контейнера
        //      variant<Type<3>, Type<4>, Type<5>>
        // Элементы контейнера
        //      map[3] = Type<3>
        //      map[4] = Type<4>
        //      map[5] = Type<5>
        //

        // Объекты поддерживаются только у одного измерения. При загрузке
        // объекта одного измерения все объекты других измерений удаляются.
        std::unordered_map<int, SequenceVariant1<MIN, MAX, ObjectStorageDimension, MeshFloat>> m_objects;

        void clear_all_data()
        {
                for (auto& p : m_objects)
                {
                        std::visit([&](auto& v) { v.clear_all_data(); }, p.second);
                }
        }

        void set_object_size_and_position(double size, const vec3& position) override
        {
                for (auto& p : m_objects)
                {
                        std::visit([&](auto& v) { v.set_object_size_and_position(size, position); }, p.second);
                }
        }

        void check_dimension(int dimension)
        {
                if (dimension < MIN || dimension > MAX)
                {
                        error("Error repository object dimension " + to_string(dimension) +
                              ", min = " + to_string(MIN) + ", max = " + to_string(MAX));
                }
        }

        std::vector<RepositoryObjects> repository_point_object_names() const override
        {
                std::vector<RepositoryObjects> names;

                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) { names.emplace_back(p.first, v.repository_point_object_names()); },
                                p.second);
                }

                return names;
        }

        bool manifold_constructor_exists() const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.manifold_constructor_exists() ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many manifold constructors " + to_string(count));
                }
                return count > 0;
        }

        bool object_exists(ObjectId id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.object_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many objects " + to_string(count));
                }
                return count > 0;
        }

        bool mesh_exists(ObjectId id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.mesh_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many meshes " + to_string(count));
                }
                return count > 0;
        }

        ObjectVariant object(ObjectId id) const override
        {
                if (!object_exists(id))
                {
                        error("No object");
                }
                std::optional<ObjectVariant> object_opt;
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.object_exists(id))
                                        {
                                                if (!object_opt)
                                                {
                                                        auto ptr = v.object(id);
                                                        if (!ptr)
                                                        {
                                                                error("Null object pointer");
                                                        }
                                                        object_opt = std::move(ptr);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error object count " + to_string(count));
                }
                ASSERT(object_opt);
                return *object_opt;
        }

        MeshVariant mesh(ObjectId id) const override
        {
                if (!mesh_exists(id))
                {
                        error("No mesh");
                }
                std::optional<MeshVariant> mesh_opt;
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.mesh_exists(id))
                                        {
                                                if (!mesh_opt)
                                                {
                                                        auto ptr = v.mesh(id);
                                                        if (!ptr)
                                                        {
                                                                error("Null mesh pointer");
                                                        }
                                                        mesh_opt = std::move(ptr);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error mesh count " + to_string(count));
                }
                ASSERT(mesh_opt);
                return *mesh_opt;
        }

        void compute_bound_cocone(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                double rho,
                double alpha) override
        {
                if (!manifold_constructor_exists())
                {
                        error("No manifold constructor");
                }
                int count = 0;
                for (auto& p : m_objects)
                {
                        std::visit(
                                [&](auto& v) {
                                        if (v.manifold_constructor_exists())
                                        {
                                                if (count == 0)
                                                {
                                                        v.compute_bound_cocone(objects, progress_list, rho, alpha);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error manifold constructor count " + to_string(count));
                }
        }

        void load_from_file(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double rho,
                double alpha) override
        {
                int dimension = file_dimension(file_name);

                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() { clear_all_data(); };
                std::visit(
                        [&](auto& v) {
                                v.load_from_file(objects, progress_list, file_name, rho, alpha, clear_function);
                        },
                        repository);
        }

        void load_from_repository(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                int dimension,
                const std::string& object_name,
                double rho,
                double alpha,
                int point_count) override
        {
                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() { clear_all_data(); };
                std::visit(
                        [&](auto& v) {
                                v.load_from_repository(
                                        objects, progress_list, object_name, rho, alpha, point_count, clear_function);
                        },
                        repository);
        }

        void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const override
        {
                if (!object_exists(id))
                {
                        error("No object");
                }
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.object_exists(id))
                                        {
                                                if (count == 0)
                                                {
                                                        v.save_to_file(id, file_name, name);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error object count " + to_string(count));
                }
        }

        std::vector<FileFormat> formats_for_save(unsigned dimension) const override
        {
                std::vector<FileFormat> v(1);

                v[0].name = "OBJ Files";
                v[0].extensions = {obj_file_extension(dimension)};

                return v;
        }

        std::vector<FileFormat> formats_for_load() const override
        {
                std::set<unsigned> dimensions;
                for (int n = MIN; n <= MAX; ++n)
                {
                        dimensions.insert(n);
                }

                std::vector<FileFormat> v(1);

                v[0].name = "All Supported Formats";
                for (std::string& s : obj_file_supported_extensions(dimensions))
                {
                        v[0].extensions.push_back(std::move(s));
                }
                for (std::string& s : txt_file_supported_extensions(dimensions))
                {
                        v[0].extensions.push_back(std::move(s));
                }

                return v;
        }

        template <size_t... I>
        void init_map(
                int mesh_threads,
                const ObjectStorageCallback& event_emitter,
                const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler,
                std::integer_sequence<size_t, I...>&&)
        {
                static_assert(((I >= 0 && I < sizeof...(I)) && ...));
                static_assert(MIN + sizeof...(I) == MAX + 1);

                (m_objects.try_emplace(
                         MIN + I, std::in_place_type_t<ObjectStorageDimension<MIN + I, MeshFloat>>(), mesh_threads,
                         event_emitter, exception_handler),
                 ...);

                ASSERT((m_objects.count(MIN + I) == 1) && ...);
                ASSERT(m_objects.size() == COUNT);
        }

public:
        ObjectStorageImpl(
                int mesh_threads,
                const ObjectStorageCallback& event_emitter,
                const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
        {
                init_map(mesh_threads, event_emitter, exception_handler, std::make_integer_sequence<size_t, COUNT>());
        }
};

std::unique_ptr<ObjectStorage> create_object_storage(
        int mesh_threads,
        const ObjectStorageCallback& event_emitter,
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
{
        return std::make_unique<ObjectStorageImpl>(mesh_threads, event_emitter, exception_handler);
}
