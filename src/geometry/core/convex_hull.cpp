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

#include "convex_hull.h"

#include "convex_hull/compute.h"
#include "convex_hull/facet.h"
#include "convex_hull/integer_types.h"
#include "convex_hull/source_points.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <array>
#include <vector>

namespace ns::geometry::core
{
namespace
{
namespace ch = convex_hull;

template <typename PointType, std::size_t N, typename SourceType>
std::vector<PointType> create_points_paraboloid(const std::vector<Vector<N, SourceType>>& points)
{
        static_assert(std::tuple_size_v<PointType> == N + 1);

        std::vector<PointType> res(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                res[i][N] = 0;
                for (std::size_t n = 0; n < N; ++n)
                {
                        res[i][n] = points[i][n];
                        // multipication using data type of the 'data'
                        res[i][N] += res[i][n] * res[i][n];
                }
        }
        return res;
}

template <typename PointType, std::size_t N, typename SourceType>
std::vector<PointType> create_points(const std::vector<Vector<N, SourceType>>& points)
{
        static_assert(std::tuple_size_v<PointType> == N);

        std::vector<PointType> res(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        res[i][n] = points[i][n];
                }
        }
        return res;
}

template <std::size_t N, typename Data, typename Compute>
std::vector<DelaunaySimplex<N>> lower_convex_hull_simplices(
        const ch::DelaunayPoints<N>& points,
        const ch::FacetList<ch::Facet<N + 1, Data, Compute>>& convex_hull_facets)
{
        using S = ch::DelaunayDataType<N>;
        using C = ch::DelaunayComputeType<N>;

        const std::vector<Vector<N, S>> data = create_points<Vector<N, S>>(points.points());

        std::vector<DelaunaySimplex<N>> res;
        res.reserve(convex_hull_facets.size());

        for (const auto& facet : convex_hull_facets)
        {
                if (!facet.last_ortho_coord_is_negative())
                {
                        // not the lower convex hull
                        continue;
                }

                const std::array<int, N + 1>& vertices = facet.vertices();

                std::array<Vector<N, double>, N + 1> orthos;
                for (std::size_t r = 0; r < N + 1; ++r)
                {
                        // ortho is directed outside
                        orthos[r] = ch::Facet<N, S, C>(data, del_elem(vertices, r), vertices[r])
                                            .template ortho_fp<double>();
                }

                res.emplace_back(points.restore_indices(vertices), orthos);
        }

        return res;
}

template <std::size_t N, typename Data, typename Compute>
std::vector<ConvexHullSimplex<N>> convex_hull_simplices(
        const ch::ConvexHullPoints<N>& points,
        const ch::FacetList<ch::Facet<N, Data, Compute>>& convex_hull_facets)
{
        std::vector<ConvexHullSimplex<N>> res;
        res.reserve(convex_hull_facets.size());
        for (const auto& facet : convex_hull_facets)
        {
                res.emplace_back(points.restore_indices(facet.vertices()), facet.template ortho_fp<double>());
        }
        return res;
}

template <std::size_t N>
std::vector<DelaunaySimplex<N>> compute_delaunay(const ch::DelaunayPoints<N>& points, progress::Ratio* const progress)
{
        using S = ch::DelaunayParaboloidDataType<N + 1>;
        using C = ch::DelaunayParaboloidComputeType<N + 1>;

        const ch::FacetList<ch::Facet<N + 1, S, C>> facets =
                ch::compute_convex_hull<C>(create_points_paraboloid<Vector<N + 1, S>>(points.points()), progress);

        return lower_convex_hull_simplices(points, facets);
}

template <std::size_t N>
std::vector<ConvexHullSimplex<N>> compute_convex_hull(
        const ch::ConvexHullPoints<N>& points,
        progress::Ratio* const progress)
{
        using S = ch::ConvexHullDataType<N>;
        using C = ch::ConvexHullComputeType<N>;

        const ch::FacetList<ch::Facet<N, S, C>> facets =
                ch::compute_convex_hull<C>(create_points<Vector<N, S>>(points.points()), progress);

        return convex_hull_simplices(points, facets);
}
}

template <std::size_t N>
DelaunayData<N> compute_delaunay(
        const std::vector<Vector<N, float>>& points,
        progress::Ratio* const progress,
        const bool write_log)
{
        if (points.empty())
        {
                error("No points to compute delaunay");
        }

        if (write_log)
        {
                LOG("Delaunay in " + space_name(N + 1) + " integer");
        }

        const ch::DelaunayPoints<N> delaunay_points(points);

        if (write_log)
        {
                LOG(ch::delaunay_type_description<N>());
        }

        std::vector<DelaunaySimplex<N>> simplices = compute_delaunay(delaunay_points, progress);

        std::vector<Vector<N, double>> result_points(points.size(), Vector<N, double>(0));
        for (std::size_t i = 0; i < delaunay_points.points().size(); ++i)
        {
                result_points[delaunay_points.restore_index(i)] = to_vector<double>(delaunay_points.points()[i]);
        }

        if (write_log)
        {
                LOG("Delaunay in " + space_name(N + 1) + " integer done");
        }

        return {.points = std::move(result_points), .simplices = std::move(simplices)};
}

template <std::size_t N>
std::vector<ConvexHullSimplex<N>> compute_convex_hull(
        const std::vector<Vector<N, float>>& points,
        progress::Ratio* const progress,
        const bool write_log)
{
        if (points.empty())
        {
                error("No data to compute convex hull");
        }

        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer");
        }

        const ch::ConvexHullPoints<N> convex_hull_points(points);

        if (write_log)
        {
                LOG(ch::convex_hull_type_description<N>());
        }

        std::vector<ConvexHullSimplex<N>> res = compute_convex_hull(convex_hull_points, progress);

        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer done");
        }

        return res;
}

#define TEMPLATE_DELAUNAY(N) \
        template DelaunayData<N> compute_delaunay(const std::vector<Vector<(N), float>>&, progress::Ratio*, bool);

#define TEMPLATE_CONVEX_HULL(N)                                           \
        template std::vector<ConvexHullSimplex<(N)>> compute_convex_hull( \
                const std::vector<Vector<(N), float>>&, progress::Ratio*, bool);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE_DELAUNAY)
TEMPLATE_INSTANTIATION_N_2_A(TEMPLATE_CONVEX_HULL)
}
