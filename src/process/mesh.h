/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/graph/mst.h>
#include <src/geometry/reconstruction/cocone.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/vec.h>
#include <src/progress/progress_list.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ns::process
{
namespace process_implementation
{
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

template <std::size_t N>
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

        TimePoint start_time = time();

        geometry::compute_convex_hull(points, &convex_hull_facets, progress, true);

        LOG("Convex hull created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        std::vector<std::array<int, N>> facets;
        facets.clear();
        facets.reserve(convex_hull_facets.size());
        for (const geometry::ConvexHullFacet<N>& f : convex_hull_facets)
        {
                facets.push_back(f.vertices());
        }

        return mesh::create_mesh_for_facets(points, facets);
}

template <std::size_t N>
void convex_hull(ProgressRatioList* progress_list, const mesh::Reading<N>& object)
{
        std::unique_ptr<const mesh::Mesh<N>> ch_mesh;
        {
                ProgressRatio progress(progress_list);
                progress.set_text(object.name() + " convex hull in " + space_name(N) + ": %v of %m");

                ch_mesh = mesh_convex_hull(object.mesh(), &progress);
        }
        if (ch_mesh->facets.empty())
        {
                return;
        }

        const std::shared_ptr<mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(ch_mesh), object.matrix(), "Convex Hull");

        obj->insert(object.id());
}

template <std::size_t N>
void cocone(
        ProgressRatioList* progress_list,
        ObjectId parent_id,
        const geometry::ManifoldConstructor<N>& constructor,
        const Matrix<N + 1, N + 1, double>& model_matrix)
{
        std::unique_ptr<const mesh::Mesh<N>> cocone_mesh;
        {
                ProgressRatio progress(progress_list);

                TimePoint start_time = time();

                std::vector<Vector<N, double>> normals;
                std::vector<std::array<int, N>> facets;

                constructor.cocone(&normals, &facets, &progress);

                cocone_mesh = mesh::create_mesh_for_facets(constructor.points(), facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }
        if (cocone_mesh->facets.empty())
        {
                return;
        }

        const std::shared_ptr<mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(cocone_mesh), model_matrix, "Cocone");

        obj->insert(parent_id);
}

template <std::size_t N>
void bound_cocone(
        ProgressRatioList* progress_list,
        ObjectId parent_id,
        const geometry::ManifoldConstructor<N>& constructor,
        const Matrix<N + 1, N + 1, double>& model_matrix,
        double rho,
        double alpha)
{
        std::unique_ptr<const mesh::Mesh<N>> bound_cocone_mesh;
        {
                ProgressRatio progress(progress_list);

                TimePoint start_time = time();

                std::vector<Vector<N, double>> normals;
                std::vector<std::array<int, N>> facets;

                constructor.bound_cocone(rho, alpha, &normals, &facets, &progress);

                bound_cocone_mesh = mesh::create_mesh_for_facets(constructor.points(), facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }
        if (bound_cocone_mesh->facets.empty())
        {
                return;
        }

        std::string name = "Bound Cocone (" + bound_cocone_text_rho_alpha(rho, alpha) + ")";
        const std::shared_ptr<mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(bound_cocone_mesh), model_matrix, name);

        obj->insert(parent_id);
}

template <std::size_t N>
void mst(
        ProgressRatioList* progress_list,
        ObjectId parent_id,
        const geometry::ManifoldConstructor<N>& constructor,
        const Matrix<N + 1, N + 1, double>& model_matrix)
{
        std::vector<std::array<int, 2>> mst_lines;
        {
                ProgressRatio progress(progress_list);

                mst_lines = geometry::minimum_spanning_tree(
                        constructor.points(), constructor.delaunay_objects(), &progress);
        }
        std::unique_ptr<const mesh::Mesh<N>> mst_mesh = mesh::create_mesh_for_lines(constructor.points(), mst_lines);
        if (mst_mesh->lines.empty())
        {
                return;
        }

        const std::shared_ptr<mesh::MeshObject<N>> obj =
                std::make_shared<mesh::MeshObject<N>>(std::move(mst_mesh), model_matrix, "MST");

        obj->insert(parent_id);
}

template <std::size_t N>
std::unique_ptr<geometry::ManifoldConstructor<N>> create_manifold_constructor(
        ProgressRatioList* progress_list,
        const std::vector<Vector<N, float>>& points)
{
        ProgressRatio progress(progress_list);
        TimePoint start_time = time();

        std::unique_ptr<geometry::ManifoldConstructor<N>> manifold_constructor =
                geometry::create_manifold_constructor(points, &progress);

        LOG("Manifold constructor created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return manifold_constructor;
}

template <std::size_t N>
void manifold_constructor(
        ProgressRatioList* progress_list,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        const Matrix<N + 1, N + 1, double>& matrix,
        const ObjectId& id,
        const std::vector<Vector<N, float>>& points,
        double rho,
        double alpha)
{
        if (!build_cocone && !build_bound_cocone && !build_mst)
        {
                return;
        }

        std::unique_ptr<const geometry::ManifoldConstructor<N>> manifold_constructor =
                create_manifold_constructor(progress_list, points);

        ThreadsWithCatch threads(3);
        try
        {
                if (build_cocone)
                {
                        threads.add(
                                [&]()
                                {
                                        cocone(progress_list, id, *manifold_constructor, matrix);
                                });
                }

                if (build_bound_cocone)
                {
                        threads.add(
                                [&]()
                                {
                                        bound_cocone(progress_list, id, *manifold_constructor, matrix, rho, alpha);
                                });
                }

                if (build_mst)
                {
                        threads.add(
                                [&]()
                                {
                                        mst(progress_list, id, *manifold_constructor, matrix);
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

template <std::size_t N>
void compute(
        ProgressRatioList* progress_list,
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        const mesh::MeshObject<N>& mesh_object,
        double rho,
        double alpha)
{
        namespace impl = process_implementation;

        ThreadsWithCatch threads(2);
        try
        {
                if (build_convex_hull)
                {
                        threads.add(
                                [&]()
                                {
                                        mesh::Reading reading(mesh_object);
                                        impl::convex_hull(progress_list, reading);
                                });
                }

                if (build_cocone || build_bound_cocone || build_mst)
                {
                        threads.add(
                                [&]()
                                {
                                        std::optional<Matrix<N + 1, N + 1, double>> matrix;
                                        std::optional<ObjectId> id;
                                        std::vector<Vector<N, float>> points;
                                        {
                                                mesh::Reading reading(mesh_object);
                                                points = !reading.mesh().facets.empty()
                                                                 ? unique_facet_vertices(reading.mesh())
                                                                 : unique_point_vertices(reading.mesh());
                                                matrix = reading.matrix();
                                                id = reading.id();
                                        }
                                        impl::manifold_constructor(
                                                progress_list, build_cocone, build_bound_cocone, build_mst, *matrix,
                                                *id, points, rho, alpha);
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
