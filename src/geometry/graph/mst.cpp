/*
Copyright (C) 2017-2024 Topological Manifold

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
Robert Sedgewick, Kevin Wayne.
Algorithms. Fourth edition.
Pearson Education, 2011.

4.3 Minimum Spanning Trees
Kruskal’s algorithm
*/

#include "mst.h"

#include <src/com/alg.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/union_find.h>
#include <src/geometry/core/ridge.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::graph
{
namespace
{
using Edge2 = core::Ridge<3>;

class WeightedEdge final
{
        double weight_;
        Edge2 edge_;

public:
        template <std::size_t N>
        WeightedEdge(const std::vector<Vector<N, float>>& points, const Edge2& edge)
                : weight_(to_vector<double>(points[edge.vertices()[1]] - points[edge.vertices()[0]]).norm_squared()),
                  edge_(edge)
        {
        }

        [[nodiscard]] double weight() const
        {
                return weight_;
        }

        [[nodiscard]] int vertex(const int index) const
        {
                return edge_.vertices()[index];
        }
};

template <std::size_t N>
std::vector<Edge2> all_edges_from_delaunay_objects(const std::vector<std::array<int, N>>& delaunay_objects)
{
        static_assert(N >= 3);

        std::vector<Edge2> edges;

        for (const std::array<int, N>& indices : delaunay_objects)
        {
                for (std::size_t p1 = 0; p1 < indices.size() - 1; ++p1)
                {
                        for (std::size_t p2 = p1 + 1; p2 < indices.size(); ++p2)
                        {
                                if (indices[p1] < indices[p2])
                                {
                                        edges.emplace_back(std::array<int, 2>{indices[p1], indices[p2]});
                                }
                                else if (indices[p1] > indices[p2])
                                {
                                        edges.emplace_back(std::array<int, 2>{indices[p2], indices[p1]});
                                }
                                else
                                {
                                        error("Double vertex in Delaunay " + to_string(indices));
                                }
                        }
                }
        }

        return edges;
}

template <std::size_t N>
std::vector<WeightedEdge> weight_edges(const std::vector<Vector<N, float>>& points, const std::vector<Edge2>& edges)
{
        std::vector<WeightedEdge> res;
        res.reserve(edges.size());
        for (const Edge2& edge : edges)
        {
                res.emplace_back(points, edge);
        }
        return res;
}

std::vector<std::array<int, 2>> kruskal(
        const int point_count,
        const int vertex_count,
        const std::vector<WeightedEdge>& sorted_edges)
{
        ASSERT(point_count > 1 && vertex_count > 1);

        const unsigned mst_size = vertex_count - 1;

        std::vector<std::array<int, 2>> mst;

        mst.reserve(mst_size);

        UnionFind<int> union_find(point_count);

        for (std::size_t i = 0; i < sorted_edges.size() && mst.size() < mst_size; ++i)
        {
                const int v = sorted_edges[i].vertex(0);
                const int w = sorted_edges[i].vertex(1);

                if (union_find.add_connection(v, w))
                {
                        mst.push_back({v, w});
                }
        }

        if (mst.size() != mst_size)
        {
                error("Error create minimum spanning tree. The graph is not connected.");
        }

        return mst;
}

template <std::size_t N>
unsigned unique_vertex_count(const std::vector<std::array<int, N>>& delaunay_objects)
{
        static_assert(N >= 3);

        std::vector<int> indices;

        for (const std::array<int, N>& obj : delaunay_objects)
        {
                for (const int index : obj)
                {
                        indices.push_back(index);
                }
        }

        sort_and_unique(&indices);

        return indices.size();
}
}

template <std::size_t N>
std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N + 1>>& delaunay_objects,
        progress::Ratio* const progress)
{
        // vector, sort and unique are faster than unordered_set

        LOG("Minimum spanning tree...");
        progress->set_text("Minimum spanning tree");
        const Clock::time_point start_time = Clock::now();

        progress->set(0, 5);

        std::vector<Edge2> edges = all_edges_from_delaunay_objects(delaunay_objects);

        progress->set(1, 5);

        sort_and_unique(&edges);

        progress->set(2, 5);

        std::vector<WeightedEdge> weighted_edges = weight_edges(points, edges);

        edges.clear();

        progress->set(3, 5);

        std::sort(
                weighted_edges.begin(), weighted_edges.end(),
                [](const WeightedEdge& a, const WeightedEdge& b) -> bool
                {
                        return a.weight() < b.weight();
                });

        progress->set(4, 5);

        std::vector<std::array<int, 2>> mst =
                kruskal(points.size(), unique_vertex_count(delaunay_objects), weighted_edges);

        LOG("Minimum spanning tree created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mst;
}

#define TEMPLATE(N)                                                                                   \
        template std::vector<std::array<int, 2>> minimum_spanning_tree(                               \
                const std::vector<Vector<(N), float>>&, const std::vector<std::array<int, (N) + 1>>&, \
                progress::Ratio*);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
