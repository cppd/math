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

#include "structure.h"

#include "cocone.h"

#include "com/alg.h"
#include "com/error.h"

#include <limits>

constexpr double MIN_DOUBLE = std::numeric_limits<double>::lowest();

namespace
{
// Соединения вершины с объектами Делоне и гранями объектов Делоне
struct VertexConnections
{
        struct Facet
        {
                // глобальный индекс грани
                int facet_index;
                // локальный индекс вершины грани, являющейся данной вершиной
                int vertex_index;
                Facet(int facet_index_, int vertex_index_) : facet_index(facet_index_), vertex_index(vertex_index_)
                {
                }
        };
        std::vector<int> objects;
        std::vector<Facet> facets;
};

//   Если вершина находится на краю объекта, то вектором положительного полюса
// считается сумма перпендикуляров односторонних граней.
//   Если вершина не находится на краю объекта, то вектором положительного полюса
// считается вектор от вершины к наиболее удалённой вершине ячейки Вороного.
//   В книге это Definition 4.1 (Poles).
template <size_t N>
vec<N> voronoi_positive_norm(const vec<N>& vertex, const std::vector<DelaunayObject<N>>& delaunay_objects,
                             const std::vector<DelaunayFacet<N>>& delaunay_facets, const VertexConnections& vertex_connections)
{
        bool unbounded = false;
        for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
        {
                if (delaunay_facets[vertex_facet.facet_index].one_sided())
                {
                        unbounded = true;
                        break;
                }
        }

        vec<N> positive_norm;

        if (unbounded)
        {
                vec<N> sum(0);
                for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
                {
                        if (delaunay_facets[vertex_facet.facet_index].one_sided())
                        {
                                sum = sum + delaunay_facets[vertex_facet.facet_index].get_ortho();
                        }
                }

                positive_norm = normalize(sum);
        }
        else
        {
                double max_distance = MIN_DOUBLE;
                vec<N> max_vector(0);
                for (int object_index : vertex_connections.objects)
                {
                        vec<N> voronoi_vertex = delaunay_objects[object_index].get_voronoi_vertex();
                        vec<N> vp = voronoi_vertex - vertex;
                        double distance = dot(vp, vp);
                        if (distance > max_distance)
                        {
                                max_distance = distance;
                                max_vector = vp;
                        }
                }

                positive_norm = normalize(max_vector);
        }

        if (!is_finite(positive_norm))
        {
                error("Positive pole vector not finite");
        }

        return positive_norm;
}

//   Вектором отрицательного полюса считается вектор от вершины к наиболее
// удалённой вершине ячейки Вороного, при котором угол между этим вектором
// и вектором к положительному полюсу составляет более 90 градусов.
//   Высотой ячейки Вороного является длина отрицательного полюса.
//   В книге это Definition 4.1 (Poles) и Definition 5.3.
template <size_t N>
double voronoi_height(const vec<N>& vertex, const std::vector<DelaunayObject<N>>& delaunay_objects,
                      const vec<N>& positive_pole_norm, const std::vector<int>& vertex_objects)
{
        double max_distance = MIN_DOUBLE;
        // vec<N> negative_pole(0);
        bool found = false;

        for (int object_index : vertex_objects)
        {
                vec<N> voronoi_vertex = delaunay_objects[object_index].get_voronoi_vertex();
                vec<N> vp = voronoi_vertex - vertex;

                if (dot(vp, positive_pole_norm) >= 0)
                {
                        continue;
                }

                double distance = dot(vp, vp);
                if (distance > max_distance)
                {
                        max_distance = distance;
                        // negative_pole = vp;
                        found = true;
                }
        }

        if (!found)
        {
                error("Negative pole vector not found");
        }

        double len = std::sqrt(max_distance);

        if (!is_finite(len))
        {
                error("Negative pole vector not finite");
        }

        return len;
}

template <size_t N>
double voronoi_edge_radius(const std::vector<DelaunayObject<N>>& delaunay_objects, const DelaunayFacet<N>& facet,
                           const vec<N>& positive_pole, const vec<N>& pa, double pa_length, double pb_length, double cos_n_a,
                           double cos_n_b)
{
        if (facet.one_sided() && cocone_inside_or_equal(cos_n_b))
        {
                return any_max<double>;
        }

        if (!facet.one_sided() && cocone_inside_or_equal(cos_n_a, cos_n_b))
        {
                return std::max(pa_length, pb_length);
        }

        // Если вершины Вороного совпадают, то до этого места не дойдёт, так как тогда они внутри COCONE.
        // Поэтому можно брать разницу между вершинами как вектор направления от a к b.
        // Но могут быть и небольшие разницы на границах COCONE.
        vec<N> a_to_b = facet.one_sided() ? facet.get_ortho() : (delaunay_objects[facet.get_delaunay(1)].get_voronoi_vertex() -
                                                                 delaunay_objects[facet.get_delaunay(0)].get_voronoi_vertex());
        vec<N> to_intersect;
        double distance;

        if (!intersect_cocone(positive_pole, pa, a_to_b, &to_intersect, &distance))
        {
                error("cocone intersection not found");
        }

        if (cocone_inside_or_equal(cos_n_a))
        {
                return std::max(pa_length, distance);
        }
        else
        {
                return distance;
        }
}

// Радиус ячейки Вороного равен максимальному расстоянию от вершины то границ ячейки Вороного в границах COCONE.
// В книге это Definition 5.3.
template <size_t N>
void cocone_facets_and_voronoi_radius(const vec<N>& vertex, const std::vector<DelaunayObject<N>>& delaunay_objects,
                                      const std::vector<DelaunayFacet<N>>& delaunay_facets, const vec<N>& positive_pole,
                                      const VertexConnections& vertex_connections, bool find_radius,
                                      std::vector<ManifoldFacet<N>>* facet_data, double* radius)
{
        ASSERT(delaunay_facets.size() == facet_data->size());

        *radius = 0;

        for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
        {
                const DelaunayFacet<N>& facet = delaunay_facets[vertex_facet.facet_index];

                // Вектор от вершины к одной из 2 вершин Вороного грани
                vec<N> pa = delaunay_objects[facet.get_delaunay(0)].get_voronoi_vertex() - vertex;
                double pa_length = length(pa);
                double cos_n_a = dot(positive_pole, pa) / pa_length;

                // Вектор от вершины к другой из 2 вершин Вороного.
                // Если нет второй вершины, то перпендикуляр наружу.
                double pb_length;
                double cos_n_b;
                if (facet.one_sided())
                {
                        pb_length = 0;
                        cos_n_b = dot(positive_pole, facet.get_ortho());
                }
                else
                {
                        vec<N> pb = delaunay_objects[facet.get_delaunay(1)].get_voronoi_vertex() - vertex;
                        pb_length = length(pb);
                        cos_n_b = dot(positive_pole, pb) / pb_length;
                }

                if (!voronoi_edge_intersects_cocone(cos_n_a, cos_n_b))
                {
                        continue;
                }

                // Грань считается COCONE, если соответствующее ей ребро Вороного пересекает COCONE всех N вершин.
                // Найдено пересечение с COCONE одной из вершин.
                (*facet_data)[vertex_facet.facet_index].cocone_vertex[vertex_facet.vertex_index] = true;

                if (find_radius && *radius != any_max<double>)
                {
                        double edge_radius = voronoi_edge_radius(delaunay_objects, facet, positive_pole, pa, pa_length, pb_length,
                                                                 cos_n_a, cos_n_b);

                        *radius = std::max(*radius, edge_radius);
                }
        }

        ASSERT(!find_radius || (*radius > 0 && *radius <= any_max<double>));
}

template <size_t N>
void cocone_neighbors(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<ManifoldFacet<N>>& facet_data,
                      const std::vector<VertexConnections>& vertex_connections, std::vector<ManifoldVertex<N>>* vertex_data)
{
        ASSERT(delaunay_facets.size() == facet_data.size());
        ASSERT(vertex_connections.size() == vertex_data->size());

        int vertex_count = vertex_connections.size();

        for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
                for (const VertexConnections::Facet& vertex_facet : vertex_connections[vertex_index].facets)
                {
                        int facet_index = vertex_facet.facet_index;
                        unsigned skip_v = vertex_facet.vertex_index;

                        for (unsigned v = 0; v < N; ++v)
                        {
                                if (v == skip_v)
                                {
                                        // Эта вершина грани совпадает с рассматриваемой вершиной, поэтому пропустить
                                        ASSERT(delaunay_facets[facet_index].get_vertices()[v] == vertex_index);
                                        continue;
                                }

                                // Если грань попадает в COCONE вершины, то включить эту вершину в список соседей COCONE
                                if (facet_data[facet_index].cocone_vertex[v])
                                {
                                        (*vertex_data)[vertex_index].cocone_neighbors.push_back(
                                                delaunay_facets[facet_index].get_vertices()[v]);
                                }
                        }
                }

                sort_and_unique(&(*vertex_data)[vertex_index].cocone_neighbors);
        }
}

template <size_t N>
void vertex_connections(int vertex_count, const std::vector<DelaunayObject<N>>& objects,
                        const std::vector<DelaunayFacet<N>>& facets, std::vector<VertexConnections>* conn)
{
        conn->clear();
        conn->resize(vertex_count);

        for (unsigned facet = 0; facet < facets.size(); ++facet)
        {
                int local_index = -1;
                for (int vertex : facets[facet].get_vertices())
                {
                        (*conn)[vertex].facets.emplace_back(facet, ++local_index);
                }
        }

        for (unsigned object = 0; object < objects.size(); ++object)
        {
                for (int vertex : objects[object].get_vertices())
                {
                        (*conn)[vertex].objects.emplace_back(object);
                }
        }
}
}

template <size_t N>
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<N>>& points,
                           const std::vector<DelaunayObject<N>>& objects, const std::vector<DelaunayFacet<N>>& facets,
                           std::vector<ManifoldVertex<N>>* vertex_data, std::vector<ManifoldFacet<N>>* facet_data)
{
        std::vector<VertexConnections> connections;

        vertex_connections(points.size(), objects, facets, &connections);

        vertex_data->clear();
        vertex_data->reserve(points.size());

        facet_data->clear();
        facet_data->resize(facets.size());

        for (unsigned v = 0; v < points.size(); ++v)
        {
                if (connections[v].facets.size() == 0 && connections[v].objects.size() == 0)
                {
                        // Не все исходные точки становятся вершинами в Делоне.
                        // Выпуклая оболочка может пропустить некоторые точки (одинаковые, близкие и т.д.).
                        vertex_data->emplace_back(vec<N>(0), 0, 0);
                        continue;
                }

                ASSERT((connections[v].facets.size() > 0) && (connections[v].objects.size() > 0));

                vec<N> positive_norm = voronoi_positive_norm(points[v], objects, facets, connections[v]);

                double radius;

                if (!find_all_vertex_data)
                {
                        cocone_facets_and_voronoi_radius(points[v], objects, facets, positive_norm, connections[v],
                                                         false /*find_radius*/, facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, 0, 0);
                }
                else
                {
                        double height = voronoi_height(points[v], objects, positive_norm, connections[v].objects);

                        cocone_facets_and_voronoi_radius(points[v], objects, facets, positive_norm, connections[v],
                                                         true /*find_radius*/, facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, height, radius);
                }
        }

        if (find_all_vertex_data)
        {
                cocone_neighbors(facets, *facet_data, connections, vertex_data);
        }

        ASSERT(vertex_data->size() == points.size());
}

// clang-format off
template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<2>>& points,
                           const std::vector<DelaunayObject<2>>& delaunay_objects,
                           const std::vector<DelaunayFacet<2>>& delaunay_facets, std::vector<ManifoldVertex<2>>* vertex_data,
                           std::vector<ManifoldFacet<2>>* facet_data);
template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<3>>& points,
                           const std::vector<DelaunayObject<3>>& delaunay_objects,
                           const std::vector<DelaunayFacet<3>>& delaunay_facets, std::vector<ManifoldVertex<3>>* vertex_data,
                           std::vector<ManifoldFacet<3>>* facet_data);
template
void vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<4>>& points,
                           const std::vector<DelaunayObject<4>>& delaunay_objects,
                           const std::vector<DelaunayFacet<4>>& delaunay_facets, std::vector<ManifoldVertex<4>>* vertex_data,
                           std::vector<ManifoldFacet<4>>* facet_data);
// clang-format on
