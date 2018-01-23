/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/error.h"
#include "com/log.h"
#include "com/time.h"
#include "geometry/graph/mst.h"
#include "obj/obj_alg.h"
#include "obj/obj_convex_hull.h"
#include "obj/obj_file_load.h"
#include "obj/obj_lines.h"
#include "obj/obj_points.h"
#include "obj/obj_surface.h"
#include "progress/progress.h"

MainObjects::MainObjects(int mesh_object_threads, std::function<void(std::function<void(std::string*)>&&)>&& catch_all,
                         const WindowEventEmitter* emitter, int point_count)
        : m_mesh_object_threads(mesh_object_threads),
          m_object_repository(create_object_repository<3>()),
          m_catch_all(std::move(catch_all)),
          m_event_emitter(emitter),
          m_point_count(point_count)
{
}

void MainObjects::catch_all(std::function<void(std::string*)>&& f) const
{
        m_catch_all(std::move(f));
}

std::vector<std::string> MainObjects::list_of_repository_point_objects() const
{
        return m_object_repository->get_list_of_point_objects();
}

void MainObjects::set_show(IShow* show)
{
        m_show = show;
}

std::shared_ptr<const Mesh> MainObjects::get_mesh(int id) const
{
        return m_meshes.get(id);
}

std::shared_ptr<const IObj> MainObjects::get_surface_cocone() const
{
        return m_surface_cocone;
}

std::shared_ptr<const IObj> MainObjects::get_surface_bound_cocone() const
{
        return m_surface_bound_cocone;
}

bool MainObjects::surface_constructor_exists() const
{
        return m_surface_constructor.get() != nullptr;
}

std::string MainObjects::object_name(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return "Model";
        case ObjectType::Cocone:
                return "COCONE";
        case ObjectType::BoundCocone:
                return "BOUND COCONE";
        }
        error_fatal("Unknown object type");
}

int MainObjects::object_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return OBJECT_MODEL;
        case ObjectType::Cocone:
                return OBJECT_COCONE;
        case ObjectType::BoundCocone:
                return OBJECT_BOUND_COCONE;
        };
        error_fatal("Unknown object type");
}

int MainObjects::convex_hull_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return OBJECT_MODEL_CONVEX_HULL;
        case ObjectType::Cocone:
                return OBJECT_COCONE_CONVEX_HULL;
        case ObjectType::BoundCocone:
                return OBJECT_BOUND_COCONE_CONVEX_HULL;
        };
        error_fatal("Unknown object type");
}

void MainObjects::mesh(ProgressRatioList* progress_list, int id, const std::shared_ptr<const IObj>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (obj->faces().size() == 0)
        {
                return;
        }

        std::lock_guard lg(m_mesh_sequential_mutex);

        ProgressRatio progress(progress_list);

        m_meshes.set(id, std::make_shared<Mesh>(obj.get(), m_model_vertex_matrix, m_mesh_object_threads, &progress));
}

void MainObjects::add_object_and_convex_hull(ProgressRatioList* progress_list, ObjectType object_type,
                                             const std::shared_ptr<const IObj>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (!(obj->faces().size() > 0 || (object_type == ObjectType::Model && obj->points().size() > 0)))
        {
                return;
        }

        int object_id = object_identifier(object_type);
        m_show->add_object(obj, object_id, OBJECT_MODEL);

        std::shared_ptr<IObj> convex_hull;

        {
                ProgressRatio progress(progress_list);
                progress.set_text(object_name(object_type) + " convex hull 3D: %v of %m");

                convex_hull = create_convex_hull_for_obj(obj.get(), &progress);
        }

        if (convex_hull->faces().size() > 0)
        {
                int convex_hull_id = convex_hull_identifier(object_type);
                m_show->add_object(convex_hull, convex_hull_id, OBJECT_MODEL);
        }

        mesh(progress_list, convex_hull_identifier(object_type), convex_hull);
}

void MainObjects::object_and_mesh(ProgressRatioList* progress_list, ObjectType object_type,
                                  const std::shared_ptr<const IObj>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::thread thread_model([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object and convex hull";

                        add_object_and_convex_hull(progress_list, object_type, obj);
                });
        });

        std::thread thread_mesh([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " mesh";

                        mesh(progress_list, object_identifier(object_type), obj);
                });
        });

        thread_model.join();
        thread_mesh.join();
}

void MainObjects::cocone(ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<3>> normals;
                std::vector<std::array<int, 3>> facets;

                m_surface_constructor->cocone(&normals, &facets, &progress);

                m_surface_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                LOG("Surface reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        object_and_mesh(progress_list, ObjectType::Cocone, m_surface_cocone);
}

void MainObjects::bound_cocone(ProgressRatioList* progress_list, double rho, double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<3>> normals;
                std::vector<std::array<int, 3>> facets;

                m_surface_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                m_surface_bound_cocone = create_obj_for_facets(m_surface_points, normals, facets);

                m_bound_cocone_rho = rho;
                m_bound_cocone_alpha = alpha;

                LOG("Surface reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        m_show->delete_object(OBJECT_BOUND_COCONE);
        m_show->delete_object(OBJECT_BOUND_COCONE_CONVEX_HULL);

        m_meshes.reset(OBJECT_BOUND_COCONE);
        m_meshes.reset(OBJECT_BOUND_COCONE_CONVEX_HULL);

        m_event_emitter->bound_cocone_loaded(rho, alpha);

        object_and_mesh(progress_list, ObjectType::BoundCocone, m_surface_bound_cocone);
}

void MainObjects::mst(ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::vector<std::array<int, 2>> mst_lines;

        {
                ProgressRatio progress(progress_list);

                mst_lines = minimum_spanning_tree(m_surface_points, m_surface_constructor->delaunay_objects(), &progress);
        }

        std::shared_ptr<IObj> mst_obj = create_obj_for_lines(m_surface_points, mst_lines);

        if (mst_obj->lines().size() > 0)
        {
                m_show->add_object(mst_obj, OBJECT_MODEL_MST, OBJECT_MODEL);
        }
}

void MainObjects::surface_constructor(ProgressRatioList* progress_list, double rho, double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                m_surface_constructor = create_manifold_constructor(m_surface_points, &progress);

                LOG("Surface reconstruction first phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        std::thread thread_cocone([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "COCONE reconstruction";

                        cocone(progress_list);
                });
        });

        std::thread thread_bound_cocone([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "BOUND COCONE reconstruction";

                        bound_cocone(progress_list, rho, alpha);
                });
        });

        std::thread thread_mst([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Minimum spanning tree 3D";

                        mst(progress_list);
                });
        });

        thread_cocone.join();
        thread_bound_cocone.join();
        thread_mst.join();
}

void MainObjects::load_object(ProgressRatioList* progress_list, std::string object_name, SourceType source_type, double rho,
                              double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<IObj> obj;

        {
                ProgressRatio progress(progress_list);
                switch (source_type)
                {
                case SourceType::File:
                        progress.set_text("Load file: %p%");
                        obj = load_obj_from_file(object_name, &progress);
                        break;
                case SourceType::Repository:
                        progress.set_text("Load object: %p%");
                        obj = create_obj_for_points(m_object_repository->get_point_object(object_name, m_point_count));
                        break;
                }
        }

        if (obj->faces().size() == 0 && obj->points().size() == 0)
        {
                error("Faces or points not found");
        }

        if (obj->faces().size() != 0 && obj->points().size() != 0)
        {
                error("Faces and points together in one object are not supported");
        }

        m_show->delete_all_objects();

        m_surface_constructor.reset();
        m_surface_cocone.reset();
        m_surface_bound_cocone.reset();

        m_meshes.reset_all();

        m_event_emitter->file_loaded(object_name);

        m_surface_points = (obj->faces().size() > 0) ? unique_face_vertices(obj.get()) : unique_point_vertices(obj.get());

        m_model_vertex_matrix = model_vertex_matrix(obj.get(), m_show->get_object_size(), m_show->get_object_position());

        std::thread thread_model([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Object and mesh";

                        object_and_mesh(progress_list, ObjectType::Model, obj);
                });
        });

        std::thread thread_surface([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Surface constructor";

                        surface_constructor(progress_list, rho, alpha);
                });
        });

        thread_model.join();
        thread_surface.join();
}
