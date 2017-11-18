/*
Copyright (C) 2017 Topological Manifold

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

Главы 1.5 (Case Study: Union-Find) и 4.3 (Minimum Spanning Trees).
*/

#include "mst.h"

#include "ridge.h"

#include "com/alg.h"
#include "com/log.h"
#include "com/print.h"
#include "com/sort.h"
#include "com/time.h"

#include <numeric>
#include <unordered_set>

// Для данной задачи ребро состоит из 2 вершин.
// Можно использовать готовый класс как бы для трёхмерного случая с двумя вершинами.
using Edge2 = Ridge<3>;

namespace
{
class WeightedQuickUnion
{
        std::vector<int> m_id; // parent link (site indexed)
        std::vector<int> m_sz; // size of component for roots (site indexed)
        int m_count; // number of components

        int find(int p) const
        {
                // Follow links to find a root.
                while (p != m_id[p])
                {
                        p = m_id[p];
                }
                return p;
        }

public:
        WeightedQuickUnion(int N) : m_id(N), m_sz(N), m_count(N)
        {
                std::iota(m_id.begin(), m_id.end(), 0);
                std::fill(m_sz.begin(), m_sz.end(), 1);
        }

        // int count() const
        //{
        //        return m_count;
        //}

        bool connect(int p, int q)
        {
                int i = find(p);
                int j = find(q);

                if (i == j)
                {
                        return false;
                }

                // Make smaller root point to larger one.
                if (m_sz[i] < m_sz[j])
                {
                        m_id[i] = j;
                        m_sz[j] += m_sz[i];
                }
                else
                {
                        m_id[j] = i;
                        m_sz[i] += m_sz[j];
                }
                --m_count;

                return true;
        }
};

template <size_t V>
unsigned get_used_vertex_count(const std::vector<std::array<int, V>>& delaunay_objects)
{
        std::unordered_set<int> v;
        for (unsigned o = 0; o < delaunay_objects.size(); ++o)
        {
                for (int p : delaunay_objects[o])
                {
                        v.insert(p);
                }
        }
        return v.size();
}

struct WeightedEdge
{
        double m_weight;
        Edge2 m_edge;

public:
        template <size_t N>
        WeightedEdge(const std::vector<Vector<N, float>>& points, const Edge2& edge_) : m_edge(edge_)
        {
                // Вес определяется как расстояние между двумя точками
                Vector<N, float> line = points[m_edge.get_vertices()[1]] - points[m_edge.get_vertices()[0]];
                // Достаточно иметь длину в квадрате
                m_weight = dot(line, line);
        }
        double get_weight() const
        {
                return m_weight;
        }
        int get_vertex(int i) const
        {
                return m_edge.get_vertices()[i];
        }
};

template <size_t N>
std::vector<WeightedEdge> create_sorted_edges(const std::vector<Vector<N, float>>& points,
                                              const std::vector<std::array<int, N + 1>>& delaunay_objects,
                                              ProgressRatio* progress)
{
        // Помещение всего в массив с последующей сортировкой и уникальностью
        // работает быстрее, чем использование unordered_set.

        progress->set_text("MST: object %v of %m");

        std::vector<Edge2> edges;
        for (unsigned object = 0; object < delaunay_objects.size(); ++object)
        {
                if ((object & 0xfff) == 0xfff)
                {
                        progress->set(object, delaunay_objects.size());
                }

                const std::array<int, N + 1>& indices = delaunay_objects[object];

                // Все сочетания по 2 вершины из всех вершин объекта Делоне
                for (unsigned p1 = 0; p1 < indices.size() - 1; ++p1)
                {
                        for (unsigned p2 = p1 + 1; p2 < indices.size(); ++p2)
                        {
                                // С сортировкой индексов по возрастанию
                                if (indices[p1] <= indices[p2])
                                {
                                        edges.emplace_back(std::array<int, 2>{{indices[p1], indices[p2]}});
                                }
                                else
                                {
                                        edges.emplace_back(std::array<int, 2>{{indices[p2], indices[p1]}});
                                }
                        }
                }
        }

        progress->set(1, 2);

        progress->set_text("MST: edges");
        sort_and_unique(&edges);

        progress->set_text("MST: weight");
        std::vector<WeightedEdge> weighted_edges;
        weighted_edges.reserve(edges.size());
        for (const Edge2& edge : edges)
        {
                weighted_edges.emplace_back(points, edge);
        }

        progress->set_text("MST: sort");
        std::sort(weighted_edges.begin(), weighted_edges.end(),
                  [](const WeightedEdge& a, const WeightedEdge& b) -> bool { return a.get_weight() < b.get_weight(); });

        return weighted_edges;
}

std::vector<std::array<int, 2>> kruskal_mst(unsigned point_count, unsigned vertex_count,
                                            const std::vector<WeightedEdge>& sorted_edges, ProgressRatio* progress)
{
        std::vector<std::array<int, 2>> mst;
        mst.reserve(vertex_count - 1);

        WeightedQuickUnion wqn(point_count);

        progress->set_text("MST: edge %v of %m");

        for (unsigned i = 0; i < sorted_edges.size() && mst.size() < vertex_count - 1; ++i)
        {
                if ((mst.size() & 0xfff) == 0xfff)
                {
                        progress->set(mst.size(), vertex_count - 1);
                }

                int v = sorted_edges[i].get_vertex(0);
                int w = sorted_edges[i].get_vertex(1);
                if (wqn.connect(v, w))
                {
                        mst.push_back({{v, w}});
                }
        }

        if (mst.size() != vertex_count - 1)
        {
                error("Error create minimum spanning tree. The graph is not connected.");
        }

        return mst;
}
}

template <size_t N>
std::vector<std::array<int, 2>> minimum_spanning_tree(const std::vector<Vector<N, float>>& points,
                                                      const std::vector<std::array<int, N + 1>>& delaunay_objects,
                                                      ProgressRatio* progress)
{
        double start_time = get_time_seconds();

        LOG("Weight and sort edges...");

        std::vector<WeightedEdge> sorted_edges;
        sorted_edges = create_sorted_edges(points, delaunay_objects, progress);

        LOG("Kruskal...");

        // В points могут быть неиспользуемые точки, надо найти количество используемых
        unsigned vertex_count = get_used_vertex_count(delaunay_objects);

        std::vector<std::array<int, 2>> mst;
        mst = kruskal_mst(points.size(), vertex_count, sorted_edges, progress);

        LOG("MST created, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");

        return mst;
}

// clang-format off
template
std::vector<std::array<int, 2>> minimum_spanning_tree(const std::vector<vec2f>& points,
                                                      const std::vector<std::array<int, 3>>& delaunay_objects,
                                                      ProgressRatio* progress);
template
std::vector<std::array<int, 2>> minimum_spanning_tree(const std::vector<vec3f>& points,
                                                      const std::vector<std::array<int, 4>>& delaunay_objects,
                                                      ProgressRatio* progress);
template
std::vector<std::array<int, 2>> minimum_spanning_tree(const std::vector<vec4f>& points,
                                                      const std::vector<std::array<int, 5>>& delaunay_objects,
                                                      ProgressRatio* progress);
// clang-format on
