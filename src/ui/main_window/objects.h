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

#pragma once

#include "event_emitter.h"
#include "meshes.h"

#include "com/mat.h"
#include "geometry/cocone/reconstruction.h"
#include "geometry/objects/points.h"
#include "obj/obj.h"
#include "path_tracing/shapes/mesh.h"
#include "progress/progress_list.h"
#include "show/show.h"

#include <memory>
#include <mutex>
#include <thread>

class MainObjects
{
        enum class ObjectType
        {
                Model,
                Cocone,
                BoundCocone
        };

        const std::thread::id m_thread_id = std::this_thread::get_id();
        const int m_mesh_object_threads;
        const std::unique_ptr<IObjectRepository<3>> m_object_repository;
        const WindowEventEmitter& m_event_emitter;
        const int m_point_count;

        Meshes<int, const Mesh> m_meshes;
        std::mutex m_mesh_sequential_mutex;
        mat4 m_model_vertex_matrix;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        std::vector<vec3f> m_surface_points;
        std::unique_ptr<IManifoldConstructor<3>> m_surface_constructor;
        std::shared_ptr<const IObj> m_surface_cocone;
        std::shared_ptr<const IObj> m_surface_bound_cocone;

        IShow* m_show;

        template <typename F>
        void catch_all(const F& function) const noexcept;

        static std::string object_name(ObjectType object_type);
        static int object_identifier(ObjectType object_type);
        static int convex_hull_identifier(ObjectType object_type);

        void mst(ProgressRatioList* progress_list);
        void mesh(ProgressRatioList* progress_list, int id, const std::shared_ptr<const IObj>& obj);
        void add_object_and_convex_hull(ProgressRatioList* progress_list, ObjectType object_type,
                                        const std::shared_ptr<const IObj>& obj);
        void object_and_mesh(ProgressRatioList* progress_list, ObjectType object_type, const std::shared_ptr<const IObj>& obj);
        void surface_constructor(ProgressRatioList* progress_list, double rho, double alpha);
        void cocone(ProgressRatioList* progress_list);

        void load_object(ProgressRatioList* progress_list, const std::string& object_name, const std::shared_ptr<const IObj>& obj,
                         double rho, double alpha);

public:
        MainObjects(int mesh_object_threads, const WindowEventEmitter& emitter, int point_count);

        std::vector<std::string> list_of_repository_point_objects() const;

        void set_show(IShow* show);

        std::shared_ptr<const Mesh> get_mesh(int id) const;

        std::shared_ptr<const IObj> get_surface_cocone() const;
        std::shared_ptr<const IObj> get_surface_bound_cocone() const;
        bool surface_constructor_exists() const;

        void bound_cocone(ProgressRatioList* progress_list, double rho, double alpha);

        void load_from_file(ProgressRatioList* progress_list, const std::string& file_name, double rho, double alpha);
        void load_from_repository(ProgressRatioList* progress_list, const std::string& object_name, double rho, double alpha);
};
