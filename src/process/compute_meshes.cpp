/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "compute_meshes.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/graph/mst.h>
#include <src/geometry/reconstruction/cocone.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/model/object_id.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/progress/progress_list.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::process
{
namespace
{
constexpr bool WRITE_LOG = true;

template <typename T>
std::string bound_cocone_text_rho_alpha(const T rho, const T alpha)
{
        static_assert(std::is_floating_point_v<T>);
        std::string res;
        res += reinterpret_cast<const char*>(u8"ρ ") + to_string_fixed(rho, 3);
        res += "; ";
        res += reinterpret_cast<const char*>(u8"α ") + to_string_fixed(alpha, 3);
        return res;
}

template <std::size_t N>
std::unique_ptr<const model::mesh::Mesh<N>> mesh_convex_hull(
        const model::mesh::Mesh<N>& mesh,
        progress::Ratio* const progress)
{
        const std::vector<numerical::Vector<N, float>> points = [&]
        {
                if (!mesh.facets.empty())
                {
                        return unique_facet_vertices(mesh);
                }
                if (!mesh.points.empty())
                {
                        return unique_point_vertices(mesh);
                }
                error("Faces or points not found for computing convex hull object");
        }();

        const std::vector<std::array<int, N>> facets = [&]
        {
                const Clock::time_point start_time = Clock::now();

                const std::vector<geometry::core::ConvexHullSimplex<N>> ch_facets =
                        geometry::core::compute_convex_hull(points, progress, true);

                LOG("Convex hull created, " + to_string_fixed(duration_from(start_time), 5) + " s");

                std::vector<std::array<int, N>> res;
                res.reserve(ch_facets.size());
                for (const geometry::core::ConvexHullSimplex<N>& f : ch_facets)
                {
                        res.push_back(f.vertices());
                }
                return res;
        }();

        return model::mesh::create_mesh_for_facets(points, facets, WRITE_LOG);
}

template <std::size_t N>
void convex_hull(progress::RatioList* const progress_list, const model::mesh::Reading<N>& object)
{
        std::unique_ptr<const model::mesh::Mesh<N>> ch_mesh;
        {
                progress::Ratio progress(progress_list);
                progress.set_text(object.name() + " convex hull in " + space_name(N) + ": %v of %m");

                ch_mesh = mesh_convex_hull(object.mesh(), &progress);
        }
        if (ch_mesh->facets.empty())
        {
                return;
        }

        const auto obj =
                std::make_shared<model::mesh::MeshObject<N>>(std::move(ch_mesh), object.matrix(), "Convex Hull");

        obj->insert(object.id());
}

template <std::size_t N>
void cocone(
        progress::RatioList* const progress_list,
        const model::ObjectId parent_id,
        const geometry::reconstruction::ManifoldConstructor<N>& constructor,
        const numerical::Matrix<N + 1, N + 1, double>& model_matrix)
{
        std::unique_ptr<const model::mesh::Mesh<N>> mesh;

        {
                progress::Ratio progress(progress_list);

                const Clock::time_point start_time = Clock::now();

                mesh = model::mesh::create_mesh_for_facets(
                        constructor.points(), constructor.cocone(&progress), WRITE_LOG);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }

        if (mesh->facets.empty())
        {
                return;
        }

        const auto obj = std::make_shared<model::mesh::MeshObject<N>>(std::move(mesh), model_matrix, "Cocone");

        obj->insert(parent_id);
}

template <std::size_t N>
void bound_cocone(
        progress::RatioList* const progress_list,
        const model::ObjectId parent_id,
        const geometry::reconstruction::ManifoldConstructor<N>& constructor,
        const numerical::Matrix<N + 1, N + 1, double>& model_matrix,
        const double rho,
        const double alpha)
{
        std::unique_ptr<const model::mesh::Mesh<N>> mesh;

        {
                progress::Ratio progress(progress_list);

                const Clock::time_point start_time = Clock::now();

                mesh = model::mesh::create_mesh_for_facets(
                        constructor.points(), constructor.bound_cocone(rho, alpha, &progress), WRITE_LOG);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }

        if (mesh->facets.empty())
        {
                return;
        }

        const auto obj = std::make_shared<model::mesh::MeshObject<N>>(
                std::move(mesh), model_matrix, "Bound Cocone (" + bound_cocone_text_rho_alpha(rho, alpha) + ")");

        obj->insert(parent_id);
}

template <std::size_t N>
void mst(
        progress::RatioList* const progress_list,
        const model::ObjectId parent_id,
        const geometry::reconstruction::ManifoldConstructor<N>& constructor,
        const numerical::Matrix<N + 1, N + 1, double>& model_matrix)
{
        std::vector<std::array<int, 2>> mst_lines;
        {
                progress::Ratio progress(progress_list);

                mst_lines = geometry::graph::minimum_spanning_tree(
                        constructor.points(), constructor.delaunay_objects(), &progress);
        }
        std::unique_ptr<const model::mesh::Mesh<N>> mst_mesh =
                model::mesh::create_mesh_for_lines(constructor.points(), mst_lines);
        if (mst_mesh->lines.empty())
        {
                return;
        }

        const auto obj = std::make_shared<model::mesh::MeshObject<N>>(std::move(mst_mesh), model_matrix, "MST");

        obj->insert(parent_id);
}

template <std::size_t N>
std::unique_ptr<geometry::reconstruction::ManifoldConstructor<N>> create_manifold_constructor(
        progress::RatioList* const progress_list,
        const std::vector<numerical::Vector<N, float>>& points)
{
        progress::Ratio progress(progress_list);
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<geometry::reconstruction::ManifoldConstructor<N>> res =
                geometry::reconstruction::create_manifold_constructor(points, &progress);

        LOG("Manifold constructor created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return res;
}

template <std::size_t N>
void manifold_constructor(
        progress::RatioList* const progress_list,
        const bool build_cocone,
        const bool build_bound_cocone,
        const bool build_mst,
        const numerical::Matrix<N + 1, N + 1, double>& matrix,
        const model::ObjectId id,
        const std::vector<numerical::Vector<N, float>>& points,
        const double rho,
        const double alpha)
{
        if (!build_cocone && !build_bound_cocone && !build_mst)
        {
                return;
        }

        std::unique_ptr<const geometry::reconstruction::ManifoldConstructor<N>> manifold_constructor =
                create_manifold_constructor(progress_list, points);

        Threads threads(3);
        try
        {
                if (build_cocone)
                {
                        threads.add(
                                [&]
                                {
                                        cocone(progress_list, id, *manifold_constructor, matrix);
                                });
                }

                if (build_bound_cocone)
                {
                        threads.add(
                                [&]
                                {
                                        bound_cocone(progress_list, id, *manifold_constructor, matrix, rho, alpha);
                                });
                }

                if (build_mst)
                {
                        threads.add(
                                [&]
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
void compute_meshes(
        progress::RatioList* const progress_list,
        const bool build_convex_hull,
        const bool build_cocone,
        const bool build_bound_cocone,
        const bool build_mst,
        const model::mesh::MeshObject<N>& mesh_object,
        const double rho,
        const double alpha)
{
        Threads threads(2);
        try
        {
                if (build_convex_hull)
                {
                        threads.add(
                                [&]
                                {
                                        const model::mesh::Reading reading(mesh_object);
                                        convex_hull(progress_list, reading);
                                });
                }

                if (build_cocone || build_bound_cocone || build_mst)
                {
                        threads.add(
                                [&]
                                {
                                        std::optional<numerical::Matrix<N + 1, N + 1, double>> matrix;
                                        std::optional<model::ObjectId> id;
                                        std::vector<numerical::Vector<N, float>> points;
                                        {
                                                const model::mesh::Reading reading(mesh_object);
                                                points = !reading.mesh().facets.empty()
                                                                 ? unique_facet_vertices(reading.mesh())
                                                                 : unique_point_vertices(reading.mesh());
                                                matrix = reading.matrix();
                                                id = reading.id();
                                        }
                                        manifold_constructor(
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

#define TEMPLATE(N)                   \
        template void compute_meshes( \
                progress::RatioList*, bool, bool, bool, bool, const model::mesh::MeshObject<N>&, double, double);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
