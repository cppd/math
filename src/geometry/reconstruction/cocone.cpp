/*
Copyright (C) 2017-2022 Topological Manifold

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

/*
Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.

4 Surface Reconstruction
5 Undersampling
*/

#include "cocone.h"

#include "extract_manifold.h"
#include "interior.h"
#include "print.h"
#include "prune_facets.h"
#include "structure.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>

#include <unordered_set>

namespace ns::geometry
{
namespace
{
constexpr double RHO_MIN = 0;
constexpr double RHO_MAX = 1;
constexpr double ALPHA_MIN = 0;
constexpr double ALPHA_MAX = 1;

bool all_false(const std::vector<bool>& data)
{
        return std::find(data.cbegin(), data.cend(), true) == data.cend();
}

template <std::size_t N>
bool cocone_facet(const ManifoldFacet<N>& facet)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!facet.cocone_vertex[i])
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N>
std::vector<bool> find_cocone_facets(const std::vector<ManifoldFacet<N>>& facets)
{
        std::vector<bool> cocone_facets;
        cocone_facets.resize(facets.size());
        for (std::size_t i = 0; i < facets.size(); ++i)
        {
                cocone_facets[i] = cocone_facet(facets[i]);
        }
        return cocone_facets;
}

template <std::size_t N>
void create_normals_and_facets(
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets,
        const std::vector<ManifoldVertex<N>>& vertices,
        std::vector<Vector<N, double>>* const normals,
        std::vector<std::array<int, N>>* const facets)
{
        std::unordered_set<int> used_points;

        facets->clear();

        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        continue;
                }

                facets->push_back(delaunay_facets[i].vertices());

                for (const auto index : delaunay_facets[i].vertices())
                {
                        used_points.insert(index);
                }
        }

        normals->clear();
        normals->resize(vertices.size(), Vector<N, double>(0));

        for (const auto p : used_points)
        {
                (*normals)[p] = vertices[p].positive_norm;
        }
}

template <std::size_t N>
void create_voronoi_delaunay(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<Vector<N, double>>* const points,
        std::vector<DelaunayObject<N>>* const delaunay_objects,
        std::vector<DelaunayFacet<N>>* const delaunay_facets,
        ProgressRatio* const progress)
{
        std::vector<DelaunaySimplex<N>> delaunay_simplices;

        LOG("compute delaunay...");
        compute_delaunay(source_points, points, &delaunay_simplices, progress, true);

        LOG("creating delaunay objects and facets and voronoi vertices...");
        create_delaunay_objects_and_facets(*points, delaunay_simplices, delaunay_objects, delaunay_facets);
}

void check_rho_and_aplha(const double rho, const double alpha)
{
        if (!(rho > RHO_MIN && rho < RHO_MAX))
        {
                error("Rho (" + to_string(rho, 10) + ") must be in the interval (" + to_string(RHO_MIN) + ", "
                      + to_string(RHO_MAX) + ")");
        }

        if (!(alpha > ALPHA_MIN && alpha < ALPHA_MAX))
        {
                error("Alpha (" + to_string(alpha, 10) + ") must be in the interval (" + to_string(ALPHA_MIN) + ", "
                      + to_string(ALPHA_MAX) + ")");
        }
}

template <std::size_t N>
class Impl final : public ManifoldConstructor<N>, public ManifoldConstructorCocone<N>
{
        const bool cocone_only_;

        std::vector<Vector<N, float>> source_points_;
        std::vector<Vector<N, double>> points_;
        std::vector<DelaunayObject<N>> delaunay_objects_;
        std::vector<DelaunayFacet<N>> delaunay_facets_;
        std::vector<ManifoldVertex<N>> vertex_data_;
        std::vector<ManifoldFacet<N>> facet_data_;

        void common_computation(
                const std::vector<bool>& interior_vertices,
                std::vector<bool>&& cocone_facets,
                std::vector<Vector<N, double>>* const normals,
                std::vector<std::array<int, N>>* const facets,
                ProgressRatio* const progress) const
        {
                progress->set(1, 4);
                LOG("prune facets...");

                prune_facets_incident_to_sharp_ridges(points_, delaunay_facets_, interior_vertices, &cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after prune. " + to_string(N - 1)
                              + "-manifold is not reconstructable.");
                }

                progress->set(2, 4);
                LOG("extract manifold...");

                cocone_facets = extract_manifold(delaunay_objects_, delaunay_facets_, cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after manifold extraction. " + to_string(N - 1)
                              + "-manifold is not reconstructable.");
                }

                progress->set(3, 4);
                LOG("create result...");

                create_normals_and_facets(delaunay_facets_, cocone_facets, vertex_data_, normals, facets);

                ASSERT(normals->size() == points_.size());
        }

        void cocone(
                std::vector<Vector<N, double>>* const normals,
                std::vector<std::array<int, N>>* const facets,
                ProgressRatio* const progress) const override
        {
                progress->set_text("Cocone reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets = find_cocone_facets(facet_data_);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                const std::vector<bool> interior_vertices(vertex_data_.size(), true);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        // ε-sample, epsilon = 0.1
        // ρ ratio condition, rho = 1.3 * epsilon
        // α normal condition, alpha = 0.14
        void bound_cocone(
                const double rho,
                const double alpha,
                std::vector<Vector<N, double>>* const normals,
                std::vector<std::array<int, N>>* const facets,
                ProgressRatio* const progress) const override
        {
                if (cocone_only_)
                {
                        error("Manifold constructor created for Cocone and not for BoundCocone");
                }

                check_rho_and_aplha(rho, alpha);

                progress->set_text("BoundCocone reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                const std::vector<bool> interior_vertices = find_interior_vertices(rho, std::cos(alpha), vertex_data_);
                if (all_false(interior_vertices))
                {
                        error("Interior vertices not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                std::vector<bool> cocone_facets =
                        find_interior_facets(delaunay_facets_, facet_data_, interior_vertices);
                if (all_false(cocone_facets))
                {
                        error("Cocone interior facets not found. " + to_string(N - 1)
                              + "-manifold is not reconstructable.");
                }

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        std::vector<std::array<int, N + 1>> delaunay_objects() const override
        {
                std::vector<std::array<int, N + 1>> objects;
                objects.reserve(delaunay_objects_.size());
                for (const DelaunayObject<N>& d : delaunay_objects_)
                {
                        objects.push_back(d.vertices());
                }
                return objects;
        }

        const std::vector<Vector<N, float>>& points() const override
        {
                return source_points_;
        }

public:
        Impl(const std::vector<Vector<N, float>>& source_points, const bool cocone_only, ProgressRatio* const progress)
                : cocone_only_(cocone_only),
                  source_points_(source_points)
        {
                if (source_points.size() < N + 2)
                {
                        error("Error point count " + to_string(source_points.size())
                              + " for cocone manifold reconstruction in " + space_name(N));
                }

                progress->set_text("Voronoi-Delaunay: %v of %m");

                create_voronoi_delaunay(source_points, &points_, &delaunay_objects_, &delaunay_facets_, progress);

                find_vertex_and_facet_data(
                        !cocone_only_, points_, delaunay_objects_, delaunay_facets_, &vertex_data_, &facet_data_);

                ASSERT(source_points.size() == points_.size());
        }
};
}

template <std::size_t N>
std::unique_ptr<ManifoldConstructor<N>> create_manifold_constructor(
        const std::vector<Vector<N, float>>& source_points,
        ProgressRatio* const progress)
{
        return std::make_unique<Impl<N>>(source_points, false, progress);
}

template <std::size_t N>
std::unique_ptr<ManifoldConstructorCocone<N>> create_manifold_constructor_cocone(
        const std::vector<Vector<N, float>>& source_points,
        ProgressRatio* const progress)
{
        return std::make_unique<Impl<N>>(source_points, true, progress);
}

#define CREATE_MANIFOLD_CONSTRUCTOR_INSTANTIATION(N)                                    \
        template std::unique_ptr<ManifoldConstructor<(N)>> create_manifold_constructor( \
                const std::vector<Vector<(N), float>>&, ProgressRatio*);

#define CREATE_MANIFOLD_CONSTRUCTOR_COCONE_INSTANTIATION(N)                                          \
        template std::unique_ptr<ManifoldConstructorCocone<(N)>> create_manifold_constructor_cocone( \
                const std::vector<Vector<(N), float>>&, ProgressRatio*);

CREATE_MANIFOLD_CONSTRUCTOR_INSTANTIATION(2)
CREATE_MANIFOLD_CONSTRUCTOR_INSTANTIATION(3)
CREATE_MANIFOLD_CONSTRUCTOR_INSTANTIATION(4)
CREATE_MANIFOLD_CONSTRUCTOR_INSTANTIATION(5)

CREATE_MANIFOLD_CONSTRUCTOR_COCONE_INSTANTIATION(2)
CREATE_MANIFOLD_CONSTRUCTOR_COCONE_INSTANTIATION(3)
CREATE_MANIFOLD_CONSTRUCTOR_COCONE_INSTANTIATION(4)
CREATE_MANIFOLD_CONSTRUCTOR_COCONE_INSTANTIATION(5)
}
