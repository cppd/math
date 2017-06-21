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

#include <numeric>
#include <unordered_set>

// Для данной задачи ребро состоит из 2 вершин.
// Можно использовать готовый класс как бы для трёхмерного случая с двумя вершинами.
using Edge2 = Ridge<3>;

namespace
{
struct WeightedEdge2
{
        double w;
        Edge2 e;
};

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

template <size_t N>
unsigned get_vertex_count(const std::vector<DelaunayObject<N>>& delaunay_objects)
{
        std::unordered_set<int> v;
        for (unsigned o = 0; o < delaunay_objects.size(); ++o)
        {
                for (int p : delaunay_objects[o].get_vertices())
                {
                        v.insert(p);
                }
        }
        return v.size();
}

template <size_t N>
void create_weighted_and_sorted_edges(const std::vector<vec<N>>& points, const std::vector<DelaunayObject<N>>& delaunay_objects,
                                      std::vector<WeightedEdge2>* weighted_edges)
{
        weighted_edges->clear();

        std::unordered_set<Edge2> edge_set;

        for (unsigned o = 0; o < delaunay_objects.size(); ++o)
        {
                const std::array<int, N + 1>& indices = delaunay_objects[o].get_vertices();

                // Все сочетания по 2 вершины из всех вершин объекта Делоне
                for (unsigned p1 = 0; p1 < indices.size() - 1; ++p1)
                {
                        for (unsigned p2 = p1 + 1; p2 < indices.size(); ++p2)
                        {
                                Edge2 e(std::array<int, 2>{{indices[p1], indices[p2]}});

                                if (edge_set.count(e) > 0)
                                {
                                        continue;
                                }

                                edge_set.insert(e);

                                vec<N> v = points[e.get_vertices()[1]] - points[e.get_vertices()[0]];
                                // Достаточно иметь длину в квадрате
                                weighted_edges->push_back({dot(v, v), e});
                        }
                }
        }

        std::sort(weighted_edges->begin(), weighted_edges->end(),
                  [](const WeightedEdge2& a, const WeightedEdge2& b) -> bool { return a.w < b.w; });
}

// Параметр edges должен быть уже отсортирован в порядке возрастания веса
void kruskal_mst(unsigned point_count, unsigned vertex_count, const std::vector<WeightedEdge2>& edges, std::vector<int>* mst)
{
        mst->clear();

        WeightedQuickUnion wqn(point_count);

        for (unsigned i = 0; i < edges.size() && mst->size() < vertex_count - 1; ++i)
        {
                int v = edges[i].e.get_vertices()[0];
                int w = edges[i].e.get_vertices()[1];
                if (wqn.connect(v, w))
                {
                        mst->push_back(i);
                }
        }
        if (mst->size() != vertex_count - 1)
        {
                error("Error create minimum spanning tree. The graph is not connected");
        }
}
}

template <size_t N>
void minimal_spanning_tree(const std::vector<vec<N>>& points, const std::vector<DelaunayObject<N>>& delaunay_objects)
{
        // В points могут быть неиспользуемые точки, надо найти количество используемых
        unsigned vertex_count = get_vertex_count(delaunay_objects);

        std::vector<WeightedEdge2> edges;
        create_weighted_and_sorted_edges(points, delaunay_objects, &edges);

        std::vector<int> mst;
        kruskal_mst(points.size(), vertex_count, edges, &mst);
}

// clang-format off
template
void minimal_spanning_tree(const std::vector<vec<2>>& points, const std::vector<DelaunayObject<2>>& delaunay_objects);
template
void minimal_spanning_tree(const std::vector<vec<3>>& points, const std::vector<DelaunayObject<3>>& delaunay_objects);
template
void minimal_spanning_tree(const std::vector<vec<4>>& points, const std::vector<DelaunayObject<4>>& delaunay_objects);
// clang-format on
