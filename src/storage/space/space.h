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

#include "pointer_map.h"

#include "../events.h"
#include "../mesh_object.h"

#include <src/geometry/cocone/reconstruction.h>
#include <src/geometry/objects/points.h>
#include <src/model/mesh.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress_list.h>

#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

template <size_t N, typename MeshFloat>
class ObjectStorageSpace
{
        static_assert(N >= 3);

        const std::thread::id m_thread_id = std::this_thread::get_id();
        const int m_mesh_threads;

        const ObjectStorageEvents& m_event_emitter;
        std::function<void(const std::exception_ptr& ptr, const std::string& msg)> m_exception_handler;

        //

        const std::unique_ptr<ObjectRepository<N>> m_object_repository;

        PointerMap<ObjectId, const MeshObject<N>> m_objects;
        PointerMap<ObjectId, const SpatialMeshModel<N, MeshFloat>> m_meshes;
        std::unordered_map<ObjectId, std::vector<Vector<N, float>>> m_manifold_constructors_points;
        std::unordered_map<ObjectId, std::unique_ptr<ManifoldConstructor<N>>> m_manifold_constructors;

        //

        std::mutex m_mesh_sequential_mutex;

        double m_object_size = 0;
        vec3 m_object_position = vec3(0);

        template <typename F>
        void catch_all(const F& function) const;

        std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> build_mesh(
                ProgressRatioList* progress_list,
                const MeshObject<N>& object);

        void add_object_and_mesh(ProgressRatioList* progress_list, const std::shared_ptr<const MeshObject<N>>& object);

        void manifold_constructor(
                ProgressRatioList* progress_list,
                const std::unordered_set<ComputationType>& objects,
                const MeshObject<N>& object,
                double rho,
                double alpha);

        void convex_hull(ProgressRatioList* progress_list, const std::shared_ptr<const MeshObject<N>>& object);

        void cocone(
                ProgressRatioList* progress_list,
                const ManifoldConstructor<N>& constructor,
                const std::vector<Vector<N, float>>& points,
                const MeshObject<N>& object);

        void bound_cocone(
                ProgressRatioList* progress_list,
                const ManifoldConstructor<N>& constructor,
                const std::vector<Vector<N, float>>& points,
                const MeshObject<N>& object,
                double rho,
                double alpha);

        void build_mst(
                ProgressRatioList* progress_list,
                const ManifoldConstructor<N>& constructor,
                const std::vector<Vector<N, float>>& points,
                const MeshObject<N>& object);

        template <typename ObjectLoaded>
        void load_object(
                const std::unordered_set<ComputationType>& objects,
                ProgressRatioList* progress_list,
                const std::string& object_name,
                std::unique_ptr<const mesh::Mesh<N>>&& mesh,
                double rho,
                double alpha,
                const ObjectLoaded& object_loaded);

public:
        ObjectStorageSpace(
                int mesh_threads,
                const ObjectStorageEvents& event_emitter,
                const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler);

        void set_object_size_and_position(double size, const vec3& position);

        void clear_all_data();

        std::vector<std::string> repository_point_object_names() const;

        bool object_exists(ObjectId id) const;
        std::shared_ptr<const MeshObject<N>> object(ObjectId id) const;

        bool mesh_exists(ObjectId id) const;
        std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> mesh(ObjectId id) const;

        void compute_bound_cocone(ProgressRatioList* progress_list, ObjectId id, double rho, double alpha);

        void load_from_file(
                const std::unordered_set<ComputationType>& objects,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double rho,
                double alpha,
                const std::function<void()>& object_loaded);

        void load_from_repository(
                const std::unordered_set<ComputationType>& objects,
                ProgressRatioList* progress_list,
                const std::string& object_name,
                double rho,
                double alpha,
                int point_count,
                const std::function<void()>& object_loaded);

        void save(ObjectId id, const std::string& file_name, const std::string& name) const;
};

//
