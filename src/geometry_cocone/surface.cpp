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
Алгоритмы восстановления поверхностей COCONE и BOUND COCONE.

По книге

Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.
*/

#include "surface.h"

#include "cocone.h"
#include "extract_manifold.h"
#include "prune_facets.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "geometry/delaunay.h"
#include "geometry/linear_algebra.h"
#include "geometry/vec_array.h"

#include <limits>
#include <list>
#include <unordered_set>

namespace
{
template <size_t N>
struct VertexData
{
        const vec<N> positive_norm;
        // const vec<N> negative_pole;
        const double height;
        const double radius;

        std::vector<int> cocone_neighbors;

        VertexData(const vec<N>& positive_norm_, double height_, double radius_)
                : positive_norm(positive_norm_), height(height_), radius(radius_)
        {
        }
};

template <size_t N>
struct FacetData
{
        std::array<bool, N> cocone_vertex;

        FacetData() : cocone_vertex(make_array_value<bool, N>(false))
        {
        }
};

// Соединения вершины с объектами Делоне и гранями объектов Делоне
struct VertexConnections
{
        std::vector<int> objects;

        struct Facet
        {
                // глобальный индекс грани
                int facet_index;
                // локальный индекс вершины грани, являющейся данной вершиной
                unsigned char vertex_index;
                Facet(int facet_index_, unsigned char vertex_index_) : facet_index(facet_index_), vertex_index(vertex_index_)
                {
                }
        };
        std::vector<Facet> facets;
};

#if 0
template <size_t N>
void print_delaunay_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets)
{
        LOG("--delaunay facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                LOG(to_string(delaunay_facets[i].get_vertices()));
        }
        LOG("--");
}
template <size_t N>
void print_cocone_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets)
{
        LOG("--cocone facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (cocone_facets[i])
                {
                        LOG(to_string(delaunay_facets[i].get_vertices()));
                }
        }
        LOG("--");
}
template <size_t N>
void print_not_cocone_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets)
{
        LOG("--not cocone facets--");
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        LOG(to_string(delaunay_facets[i].get_vertices()));
                }
        }
        LOG("--");
}
template <size_t N>
void print_vertex_data(const std::vector<VertexData<N>>& vertex_data)
{
        LOG("--vertices--");
        for (size_t i = 0; i < vertex_data.size(); ++i)
        {
                LOG("pole " + to_string(i) + ": " + to_string(vertex_data[i].positive_norm));
        }
        LOG("--");
}
#endif

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
                double max_distance = std::numeric_limits<double>::lowest();
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
        double max_distance = std::numeric_limits<double>::lowest();
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

        double len = sqrt(max_distance);

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
                                      std::vector<FacetData<N>>* facet_data, double* radius)
{
        ASSERT(delaunay_facets.size() == facet_data->size());

        *radius = 0;

        // for (unsigned i = 0; i < vertex_connections.facets.size(); ++i)
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
void cocone_neighbors(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<FacetData<N>>& facet_data,
                      const std::vector<VertexConnections>& vertex_connections, std::vector<VertexData<N>>* vertex_data)
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
void fill_vertex_and_facet_data(bool find_all_vertex_data, const std::vector<vec<N>>& points,
                                const std::vector<DelaunayObject<N>>& delaunay_objects,
                                const std::vector<DelaunayFacet<N>>& delaunay_facets, std::vector<VertexData<N>>* vertex_data,
                                std::vector<FacetData<N>>* facet_data)
{
        std::vector<VertexConnections> vertex_connections(points.size());

        for (unsigned facet_index = 0; facet_index < delaunay_facets.size(); ++facet_index)
        {
                const std::array<int, N>& vertices = delaunay_facets[facet_index].get_vertices();
                for (unsigned i = 0; i < vertices.size(); ++i)
                {
                        vertex_connections[vertices[i]].facets.emplace_back(facet_index, i);
                }
        }

        for (unsigned object_index = 0; object_index < delaunay_objects.size(); ++object_index)
        {
                for (int vertex : delaunay_objects[object_index].get_vertices())
                {
                        vertex_connections[vertex].objects.push_back(object_index);
                }
        }

        vertex_data->clear();
        vertex_data->reserve(points.size());

        facet_data->clear();
        facet_data->resize(delaunay_facets.size());

        for (unsigned p = 0; p < points.size(); ++p)
        {
                if (vertex_connections[p].facets.size() == 0 && vertex_connections[p].objects.size() == 0)
                {
                        // Не все исходные точки становятся вершинами в Делоне.
                        // Выпуклая оболочка может пропустить некоторые точки (одинаковые, близкие и т.д.).
                        vertex_data->emplace_back(vec<N>(0), 0, 0);
                        continue;
                }

                ASSERT((vertex_connections[p].facets.size() > 0) && (vertex_connections[p].objects.size() > 0));

                vec<N> positive_norm = voronoi_positive_norm(points[p], delaunay_objects, delaunay_facets, vertex_connections[p]);

                double radius;

                if (!find_all_vertex_data)
                {
                        cocone_facets_and_voronoi_radius(points[p], delaunay_objects, delaunay_facets, positive_norm,
                                                         vertex_connections[p], false /*find_radius*/, facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, 0, 0);
                }
                else
                {
                        double height = voronoi_height(points[p], delaunay_objects, positive_norm, vertex_connections[p].objects);

                        cocone_facets_and_voronoi_radius(points[p], delaunay_objects, delaunay_facets, positive_norm,
                                                         vertex_connections[p], true /*find_radius*/, facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, height, radius);
                }
        }

        if (find_all_vertex_data)
        {
                cocone_neighbors(delaunay_facets, *facet_data, vertex_connections, vertex_data);
        }

        ASSERT(vertex_data->size() == points.size());
}

template <size_t N>
void find_cocone_facets(const std::vector<FacetData<N>>& facet_data, std::vector<bool>* cocone_facets)
{
        cocone_facets->clear();
        cocone_facets->resize(facet_data.size());

        // Грань считается COCONE, если соответствующее ей ребро Вороного пересекает COCONE всех N вершин.
        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                bool cocone = true;
                for (unsigned v = 0; v < N; ++v)
                {
                        if (!facet_data[f].cocone_vertex[v])
                        {
                                cocone = false;
                                break;
                        }
                }
                (*cocone_facets)[f] = cocone;
        }

        if (all_false(*cocone_facets))
        {
                error("Cocone facets not found. Surface is not reconstructable.");
        }
}

template <size_t N>
void find_interior_vertices(double RHO, double ALPHA, const std::vector<VertexData<N>>& vertex_data,
                            std::vector<bool>* interior_vertices)
{
        interior_vertices->clear();
        interior_vertices->resize(vertex_data.size(), false);

        int interior_count = 0;

        for (unsigned v = 0; v < vertex_data.size(); ++v)
        {
                const VertexData<N>& data = vertex_data[v];

                if (!(data.radius <= RHO * data.height))
                {
                        continue;
                }

                bool flat = true;

                // Нужно соответствие угла со всеми соседними вершинами
                for (int n : data.cocone_neighbors)
                {
                        if (!(dot(data.positive_norm, vertex_data[n].positive_norm) >= ALPHA))
                        {
                                flat = false;
                                break;
                        }
                }

                if (flat)
                {
                        (*interior_vertices)[v] = true;
                        ++interior_count;
                }
        }

        if (interior_count == 0)
        {
                error("interior points not found");
        }

        LOG("interior points after initial phase: " + to_string(interior_count) + " (" + to_string(vertex_data.size()) + ")");

        bool found;
        do
        {
                found = false;

                for (unsigned v = 0; v < vertex_data.size(); ++v)
                {
                        if ((*interior_vertices)[v])
                        {
                                continue;
                        }

                        const VertexData<N>& data = vertex_data[v];

                        if (!(data.radius <= RHO * data.height))
                        {
                                continue;
                        }

                        for (int n : data.cocone_neighbors)
                        {
                                // Достаточно соответствия угла с одной соседней вершиной, являющейся внутренней

                                if (!(*interior_vertices)[n])
                                {
                                        continue;
                                }

                                if (dot(data.positive_norm, vertex_data[n].positive_norm) >= ALPHA)
                                {
                                        (*interior_vertices)[v] = true;
                                        found = true;
                                        ++interior_count;
                                        break;
                                }
                        }
                }

        } while (found);

        LOG("interior points after expansion phase: " + to_string(interior_count) + " (" + to_string(vertex_data.size()) + ")");
}

template <size_t N>
void find_cocone_interior_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets,
                                 const std::vector<FacetData<N>>& facet_data, const std::vector<bool>& interior_vertices,
                                 std::vector<bool>* cocone_facets)
{
        ASSERT(delaunay_facets.size() == facet_data.size());

        cocone_facets->clear();
        cocone_facets->resize(delaunay_facets.size());

        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                bool cocone = true;
                bool interior_found = false;
                for (unsigned v = 0; v < N; ++v)
                {
                        bool interior_cocone =
                                interior_vertices[delaunay_facets[f].get_vertices()[v]] && facet_data[f].cocone_vertex[v];
                        bool boundary = !interior_vertices[delaunay_facets[f].get_vertices()[v]];
                        if (!(interior_cocone || boundary))
                        {
                                cocone = false;
                                break;
                        }
                        if (interior_cocone)
                        {
                                interior_found = true;
                        }
                }
                (*cocone_facets)[f] = interior_found && cocone;
        }

        if (all_false(*cocone_facets))
        {
                error("Cocone interior facets not found. Surface is not reconstructable.");
        }
}

template <size_t N>
void create_normals_and_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets,
                               const std::vector<VertexData<N>>& vertex_data, std::vector<vec<N>>* normals,
                               std::vector<std::array<int, N>>* facets)
{
        std::unordered_set<int> used_points;

        facets->clear();

        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        continue;
                }

                facets->push_back(delaunay_facets[i].get_vertices());

                for (int index : delaunay_facets[i].get_vertices())
                {
                        used_points.insert(index);
                }
        }

        normals->clear();
        normals->resize(vertex_data.size(), vec<N>(0));

        for (int p : used_points)
        {
                (*normals)[p] = vertex_data[p].positive_norm;
        }
}

template <size_t N>
void create_voronoi_delaunay(const std::vector<Vector<N, float>>& source_points, std::vector<vec<N>>* points,
                             std::vector<DelaunayObject<N>>* delaunay_objects, std::vector<DelaunayFacet<N>>* delaunay_facets,
                             ProgressRatio* progress)
{
        std::vector<DelaunaySimplex<N>> delaunay_simplices;
        LOG("compute delaunay...");
        compute_delaunay(source_points, points, &delaunay_simplices, progress);

        LOG("creating delaunay objects and facets and voronoi vertices...");
        create_delaunay_objects_and_facets(*points, delaunay_simplices, delaunay_objects, delaunay_facets);
}

template <size_t N>
class SurfaceReconstructor : public ISurfaceReconstructor<N>, public ISurfaceReconstructorCoconeOnly<N>
{
        const bool m_cocone_only;

        std::vector<vec<N>> m_points;
        std::vector<DelaunayObject<N>> m_delaunay_objects;
        std::vector<DelaunayFacet<N>> m_delaunay_facets;
        std::vector<VertexData<N>> m_vertex_data;
        std::vector<FacetData<N>> m_facet_data;

        void common_computation(const std::vector<bool>& interior_vertices, std::vector<bool>&& cocone_facets,
                                std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                                ProgressRatio* progress) const
        {
                progress->set(1, 4);
                LOG("prune facets...");

                prune_facets_incident_to_sharp_ridges(m_points, m_delaunay_facets, interior_vertices, &cocone_facets);

                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after prune");
                }

                //

                progress->set(2, 4);
                LOG("extract manifold...");

                extract_manifold(m_delaunay_objects, m_delaunay_facets, &cocone_facets);

                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after manifold extraction");
                }

                //

                progress->set(3, 4);
                LOG("create result...");

                create_normals_and_facets(m_delaunay_facets, cocone_facets, m_vertex_data, normals, facets);

                ASSERT(normals->size() == m_points.size());
        }

        void cocone(std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets, ProgressRatio* progress) const override
        {
                progress->set_text("COCONE reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets;
                std::vector<bool> interior_vertices;

                find_cocone_facets(m_facet_data, &cocone_facets);
                interior_vertices.resize(m_vertex_data.size(), true);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        // ε-sample EPSILON = 0.1;
        // ρ для отношения ширины и высоты ячейки Вороного
        // RHO = 1.3 * EPSILON;
        // α для углов между векторами к положительным полюсам ячеек Вороного
        // ALPHA = 0.14;
        void bound_cocone(double RHO, double ALPHA, std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                          ProgressRatio* progress) const override
        {
                if (m_cocone_only)
                {
                        error("Surface reconstructor created for cocone and not for bound cocone");
                }

                progress->set_text("BOUND COCONE reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets;
                std::vector<bool> interior_vertices;

                find_interior_vertices(RHO, ALPHA, m_vertex_data, &interior_vertices);
                find_cocone_interior_facets(m_delaunay_facets, m_facet_data, interior_vertices, &cocone_facets);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

public:
        SurfaceReconstructor(const std::vector<Vector<N, float>>& source_points, bool cocone_only, ProgressRatio* progress)
                : m_cocone_only(cocone_only)
        {
                // Проверить на самый минимум по количеству точек
                if (source_points.size() < N + 2)
                {
                        error("Error point count " + std::to_string(source_points.size()) +
                              " for cocone manifold reconstruction " + std::to_string(N) + "D");
                }

                progress->set_text("Voronoi-Delaunay: %v of %m");

                create_voronoi_delaunay(source_points, &m_points, &m_delaunay_objects, &m_delaunay_facets, progress);

                fill_vertex_and_facet_data(!cocone_only, m_points, m_delaunay_objects, m_delaunay_facets, &m_vertex_data,
                                           &m_facet_data);

                ASSERT(source_points.size() == m_points.size());
        }
};
}

template <size_t N>
std::unique_ptr<ISurfaceReconstructor<N>> create_surface_reconstructor(const std::vector<Vector<N, float>>& source_points,
                                                                       ProgressRatio* progress)
{
        return std::make_unique<SurfaceReconstructor<N>>(source_points, false, progress);
}

template <size_t N>
std::unique_ptr<ISurfaceReconstructorCoconeOnly<N>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<N, float>>& source_points, ProgressRatio* progress)
{
        return std::make_unique<SurfaceReconstructor<N>>(source_points, true, progress);
}

// clang-format off
template
std::unique_ptr<ISurfaceReconstructor<2>> create_surface_reconstructor(const std::vector<Vector<2, float>>& source_points,
                                                                       ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructor<3>> create_surface_reconstructor(const std::vector<Vector<3, float>>& source_points,
                                                                       ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<2>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<2, float>>& source_points, ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<3>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<3, float>>& source_points, ProgressRatio* progress);
// clang-format on
