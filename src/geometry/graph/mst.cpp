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

/*
Минимальное остовное дерево по алгоритму Крускала (Kruskal’s MST algorithm).

По книге

Robert Sedgewick, Kevin Wayne.
Algorithms. Fourth edition.
Pearson Education, 2011.

4.3 Minimum Spanning Trees.
*/

#include "mst.h"

#include "../core/ridge.h"

#include <src/com/alg.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/union_find.h>

namespace geometry
{
namespace
{
// Для данной задачи ребро состоит из 2 вершин.
// Можно использовать готовый класс как бы для трёхмерного случая с двумя вершинами.
using Edge2 = Ridge<3>;

struct WeightedEdge
{
        double m_weight;
        Edge2 m_edge;

public:
        template <size_t N>
        WeightedEdge(const std::vector<Vector<N, float>>& points, const Edge2& edge) : m_edge(edge)
        {
                // Вес определяется как расстояние между двумя точками.
                // Используется double из-за сумм квадратов.
                Vector<N, double> line = to_vector<double>(points[m_edge.vertices()[1]] - points[m_edge.vertices()[0]]);

                // Достаточно иметь длину в квадрате
                m_weight = dot(line, line);
        }
        double weight() const
        {
                return m_weight;
        }
        int vertex(int i) const
        {
                return m_edge.vertices()[i];
        }
};

template <size_t N>
std::vector<Edge2> all_edges_from_delaunay_objects(const std::vector<std::array<int, N>>& delaunay_objects)
{
        static_assert(N >= 3);

        std::vector<Edge2> edges;

        for (const std::array<int, N>& indices : delaunay_objects)
        {
                // Все сочетания по 2 вершины из всех вершин объекта Делоне
                for (unsigned p1 = 0; p1 < indices.size() - 1; ++p1)
                {
                        for (unsigned p2 = p1 + 1; p2 < indices.size(); ++p2)
                        {
                                // С индексами по возрастанию
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

template <size_t N>
std::vector<WeightedEdge> weight_edges(const std::vector<Vector<N, float>>& points, const std::vector<Edge2>& edges)
{
        std::vector<WeightedEdge> weighted_edges;

        weighted_edges.reserve(edges.size());

        for (const Edge2& edge : edges)
        {
                weighted_edges.emplace_back(points, edge);
        }

        return weighted_edges;
}

std::vector<std::array<int, 2>> kruskal(
        int point_count,
        int vertex_count,
        const std::vector<WeightedEdge>& sorted_edges)
{
        ASSERT(point_count > 1 && vertex_count > 1);

        const unsigned mst_size = vertex_count - 1;

        std::vector<std::array<int, 2>> mst;

        mst.reserve(mst_size);

        UnionFind<int> union_find(point_count);

        for (unsigned i = 0; i < sorted_edges.size() && mst.size() < mst_size; ++i)
        {
                int v = sorted_edges[i].vertex(0);
                int w = sorted_edges[i].vertex(1);

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

template <size_t N>
unsigned unique_vertex_count(const std::vector<std::array<int, N>>& delaunay_objects)
{
        static_assert(N >= 3);

        std::vector<int> v;

        for (const std::array<int, N>& obj : delaunay_objects)
        {
                for (int n : obj)
                {
                        v.push_back(n);
                }
        }

        sort_and_unique(&v);

        return v.size();
}
}

template <size_t N>
std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N + 1>>& delaunay_objects,
        ProgressRatio* progress)
{
        // Помещение всех (возможно повторяющихся) соединений вершин
        // в массив с последующей сортировкой и уникальностью работает
        // быстрее, чем помещение соединений вершин в unordered_set.

        LOG("Minimum spanning tree...");
        progress->set_text("Minimum spanning tree");
        TimePoint start_time = time();

        progress->set(0, 5);

        // Все соединения вершин из Делоне, без уникальности.
        std::vector<Edge2> edges = all_edges_from_delaunay_objects(delaunay_objects);

        progress->set(1, 5);

        // Получение уникальных соединений вершин.
        sort_and_unique(&edges);

        progress->set(2, 5);

        // Определение весов ребер графа. Вес ребра определяется как его длина.
        std::vector<WeightedEdge> weighted_edges = weight_edges(points, edges);

        edges.clear();

        progress->set(3, 5);

        // Сортировка рёбер графа по возрастанию веса.
        std::sort(
                weighted_edges.begin(), weighted_edges.end(),
                [](const WeightedEdge& a, const WeightedEdge& b) -> bool
                {
                        return a.weight() < b.weight();
                });

        progress->set(4, 5);

        // Само построение минимального остовного дерева.
        std::vector<std::array<int, 2>> mst =
                kruskal(points.size(), unique_vertex_count(delaunay_objects), weighted_edges);

        LOG("Minimum spanning tree created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mst;
}

template std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<2, float>>& points,
        const std::vector<std::array<int, 3>>& delaunay_objects,
        ProgressRatio* progress);
template std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<3, float>>& points,
        const std::vector<std::array<int, 4>>& delaunay_objects,
        ProgressRatio* progress);
template std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<4, float>>& points,
        const std::vector<std::array<int, 5>>& delaunay_objects,
        ProgressRatio* progress);
template std::vector<std::array<int, 2>> minimum_spanning_tree(
        const std::vector<Vector<5, float>>& points,
        const std::vector<std::array<int, 6>>& delaunay_objects,
        ProgressRatio* progress);
}
