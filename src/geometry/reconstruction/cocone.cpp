/*
Copyright (C) 2017-2023 Topological Manifold

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
#include "prune_facets.h"
#include "structure.h"

#include "../core/convex_hull.h"
#include "../core/delaunay.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace ns::geometry::reconstruction
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
        std::vector<bool> res(facets.size());
        for (std::size_t i = 0; i < facets.size(); ++i)
        {
                res[i] = cocone_facet(facets[i]);
        }
        return res;
}

template <std::size_t N>
std::vector<std::array<int, N>> create_facets(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& cocone_facets)
{
        std::vector<std::array<int, N>> res;
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if (cocone_facets[i])
                {
                        res.push_back(delaunay_facets[i].vertices());
                }
        }
        return res;
}

template <std::size_t N>
struct DelaunayData final
{
        std::vector<Vector<N, double>> points;
        std::vector<core::DelaunayObject<N>> objects;
        std::vector<core::DelaunayFacet<N>> facets;
};

template <std::size_t N>
DelaunayData<N> create_voronoi_delaunay(
        const std::vector<Vector<N, float>>& source_points,
        progress::Ratio* const progress)
{
        constexpr bool WRITE_LOG = true;

        LOG("computing delaunay...");
        core::DelaunayData<N> delaunay = core::compute_delaunay(source_points, progress, WRITE_LOG);

        DelaunayData<N> res;

        res.points = std::move(delaunay.points);

        LOG("creating delaunay objects...");
        res.objects = core::create_delaunay_objects(res.points, delaunay.simplices);

        LOG("creating delaunay facets...");
        res.facets = core::create_delaunay_facets(delaunay.simplices);

        return res;
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
        std::vector<core::DelaunayObject<N>> delaunay_objects_;
        std::vector<core::DelaunayFacet<N>> delaunay_facets_;
        std::vector<ManifoldVertex<N>> vertex_data_;
        std::vector<ManifoldFacet<N>> facet_data_;

        std::vector<std::array<int, N>> compute_facets(
                const std::vector<bool>& interior_vertices,
                std::vector<bool>&& cocone_facets,
                progress::Ratio* const progress) const
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

                return create_facets(delaunay_facets_, cocone_facets);
        }

        [[nodiscard]] std::vector<std::array<int, N>> cocone(progress::Ratio* const progress) const override
        {
                progress->set_text("Cocone reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                const std::vector<bool> interior_vertices(vertex_data_.size(), true);

                std::vector<bool> cocone_facets = find_cocone_facets(facet_data_);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                return compute_facets(interior_vertices, std::move(cocone_facets), progress);
        }

        // ε-sample, epsilon = 0.1
        // ρ ratio condition, rho = 1.3 * epsilon
        // α normal condition, alpha = 0.14
        [[nodiscard]] std::vector<std::array<int, N>> bound_cocone(
                const double rho,
                const double alpha,
                progress::Ratio* const progress) const override
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

                return compute_facets(interior_vertices, std::move(cocone_facets), progress);
        }

        [[nodiscard]] std::vector<std::array<int, N + 1>> delaunay_objects() const override
        {
                std::vector<std::array<int, N + 1>> res;
                res.reserve(delaunay_objects_.size());
                for (const core::DelaunayObject<N>& object : delaunay_objects_)
                {
                        res.push_back(object.vertices());
                }
                return res;
        }

        [[nodiscard]] std::vector<Vector<N, double>> normals() const override
        {
                std::vector<Vector<N, double>> res;
                res.reserve(vertex_data_.size());
                for (const ManifoldVertex<N>& vertex : vertex_data_)
                {
                        res.push_back(vertex.positive_norm);
                }
                return res;
        }

        [[nodiscard]] const std::vector<Vector<N, float>>& points() const override
        {
                return source_points_;
        }

public:
        Impl(const std::vector<Vector<N, float>>& source_points,
             const bool cocone_only,
             progress::Ratio* const progress)
                : cocone_only_(cocone_only),
                  source_points_(source_points)
        {
                if (source_points.size() < N + 2)
                {
                        error("Error point count " + to_string(source_points.size())
                              + " for cocone manifold reconstruction in " + space_name(N));
                }

                progress->set_text("Voronoi-Delaunay: %v of %m");

                {
                        DelaunayData<N> data = create_voronoi_delaunay(source_points, progress);
                        points_ = std::move(data.points);
                        delaunay_objects_ = std::move(data.objects);
                        delaunay_facets_ = std::move(data.facets);
                }

                {
                        ManifoldData<N> data =
                                find_manifold_data(!cocone_only_, points_, delaunay_objects_, delaunay_facets_);
                        vertex_data_ = std::move(data.vertices);
                        facet_data_ = std::move(data.facets);
                }

                ASSERT(source_points.size() == points_.size());
                ASSERT(source_points.size() == vertex_data_.size());
        }
};
}

template <std::size_t N>
std::unique_ptr<ManifoldConstructor<N>> create_manifold_constructor(
        const std::vector<Vector<N, float>>& source_points,
        progress::Ratio* const progress)
{
        return std::make_unique<Impl<N>>(source_points, false, progress);
}

template <std::size_t N>
std::unique_ptr<ManifoldConstructorCocone<N>> create_manifold_constructor_cocone(
        const std::vector<Vector<N, float>>& source_points,
        progress::Ratio* const progress)
{
        return std::make_unique<Impl<N>>(source_points, true, progress);
}

#define TEMPLATE(N)                                                                                  \
        template std::unique_ptr<ManifoldConstructor<(N)>> create_manifold_constructor(              \
                const std::vector<Vector<(N), float>>&, progress::Ratio*);                           \
        template std::unique_ptr<ManifoldConstructorCocone<(N)>> create_manifold_constructor_cocone( \
                const std::vector<Vector<(N), float>>&, progress::Ratio*);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
