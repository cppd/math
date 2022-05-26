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

#include "convex_hull.h"

#include "convex_hull/compute.h"
#include "convex_hull/integer_types.h"
#include "convex_hull/points.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>

#include <array>
#include <vector>

namespace ns::geometry
{
namespace
{
namespace ch = convex_hull;

template <typename PointType, std::size_t N, typename SourceType>
std::vector<PointType> create_points_paraboloid(const std::vector<Vector<N, SourceType>>& points)
{
        static_assert(std::tuple_size_v<PointType> == N + 1);

        std::vector<PointType> data(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                data[i][N] = 0;
                for (std::size_t n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                        // multipication using data type of the 'data'
                        data[i][N] += data[i][n] * data[i][n];
                }
        }
        return data;
}

template <typename PointType, std::size_t N, typename SourceType>
std::vector<PointType> create_points(const std::vector<Vector<N, SourceType>>& points)
{
        static_assert(std::tuple_size_v<PointType> == N);

        std::vector<PointType> data(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                }
        }
        return data;
}

template <std::size_t N>
std::vector<DelaunaySimplex<N>> compute_delaunay(const ch::DelaunayPoints<N>& points, ProgressRatio* const progress)
{
        using FacetCH =
                ch::Facet<N + 1, ch::DelaunayParaboloidDataType<N + 1>, ch::DelaunayParaboloidComputeType<N + 1>>;
        using PointCH = Vector<N + 1, ch::DelaunayParaboloidDataType<N + 1>>;
        using FacetDelaunay = ch::Facet<N, ch::DelaunayDataType<N>, ch::DelaunayComputeType<N>>;
        using PointDelaunay = Vector<N, ch::DelaunayDataType<N>>;

        ch::FacetList<FacetCH> convex_hull_facets;
        ch::compute_convex_hull(create_points_paraboloid<PointCH>(points.points()), &convex_hull_facets, progress);

        // compute ortho in n-space and create facets

        const std::vector<PointDelaunay> data = create_points<PointDelaunay>(points.points());

        std::vector<DelaunaySimplex<N>> simplices;
        simplices.reserve(convex_hull_facets.size());
        for (const FacetCH& facet : convex_hull_facets)
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
                        orthos[r] = FacetDelaunay(data, del_elem(vertices, r), vertices[r], nullptr).double_ortho();
                }

                simplices.emplace_back(points.restore_indices(vertices), orthos);
        }
        return simplices;
}

template <std::size_t N>
std::vector<ConvexHullFacet<N>> compute_convex_hull(
        const ch::ConvexHullPoints<N>& points,
        ProgressRatio* const progress)
{
        using Facet = ch::Facet<N, ch::ConvexHullDataType<N>, ch::ConvexHullComputeType<N>>;
        using Point = Vector<N, ch::ConvexHullDataType<N>>;

        ch::FacetList<Facet> convex_hull_facets;
        ch::compute_convex_hull(create_points<Point>(points.points()), &convex_hull_facets, progress);

        std::vector<ConvexHullFacet<N>> facets;
        facets.reserve(convex_hull_facets.size());
        for (const Facet& facet : convex_hull_facets)
        {
                facets.emplace_back(points.restore_indices(facet.vertices()), facet.double_ortho());
        }
        return facets;
}
}

template <std::size_t N>
void compute_delaunay(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<Vector<N, double>>* const points,
        std::vector<DelaunaySimplex<N>>* const simplices,
        ProgressRatio* const progress,
        const bool write_log)
{
        if (source_points.empty())
        {
                error("No points to compute delaunay");
        }

        if (write_log)
        {
                LOG("Delaunay in " + space_name(N + 1) + " integer");
        }

        const ch::DelaunayPoints<N> delaunay_points(source_points);

        if (write_log)
        {
                LOG(ch::delaunay_type_description<N>());
        }

        *simplices = compute_delaunay(delaunay_points, progress);

        points->clear();
        points->resize(source_points.size(), Vector<N, double>(0));
        for (std::size_t i = 0; i < delaunay_points.points().size(); ++i)
        {
                (*points)[delaunay_points.restore_index(i)] = to_vector<double>(delaunay_points.points()[i]);
        }

        if (write_log)
        {
                LOG("Delaunay in " + space_name(N + 1) + " integer done");
        }
}

template <std::size_t N>
void compute_convex_hull(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<ConvexHullFacet<N>>* const facets,
        ProgressRatio* const progress,
        const bool write_log)
{
        if (source_points.empty())
        {
                error("No data to compute convex hull");
        }

        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer");
        }

        const ch::ConvexHullPoints<N> convex_hull_points(source_points);

        if (write_log)
        {
                LOG(ch::convex_hull_type_description<N>());
        }

        *facets = compute_convex_hull(convex_hull_points, progress);

        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer done");
        }
}

#define COMPUTE_DELAUNAY_INSTANTIATION(N)                                                  \
        template void compute_delaunay(                                                    \
                const std::vector<Vector<(N), float>>&, std::vector<Vector<(N), double>>*, \
                std::vector<DelaunaySimplex<(N)>>*, ProgressRatio*, bool);

#define COMPUTE_CONVEX_HULL_INSTANTIATION(N) \
        template void compute_convex_hull(   \
                const std::vector<Vector<(N), float>>&, std::vector<ConvexHullFacet<(N)>>*, ProgressRatio*, bool);

COMPUTE_DELAUNAY_INSTANTIATION(2)
COMPUTE_DELAUNAY_INSTANTIATION(3)
COMPUTE_DELAUNAY_INSTANTIATION(4)
COMPUTE_DELAUNAY_INSTANTIATION(5)

COMPUTE_CONVEX_HULL_INSTANTIATION(2)
COMPUTE_CONVEX_HULL_INSTANTIATION(3)
COMPUTE_CONVEX_HULL_INSTANTIATION(4)
COMPUTE_CONVEX_HULL_INSTANTIATION(5)
COMPUTE_CONVEX_HULL_INSTANTIATION(6)
}
