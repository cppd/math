/*
Copyright (C) 2017-2021 Topological Manifold

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
*/

#include "structure.h"

#include "functions.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

namespace ns::geometry
{
namespace
{
constexpr double LIMIT_COSINE_FOR_INTERSECTION_PA_POLE = 0.99;
constexpr double LIMIT_COSINE_FOR_INTERSECTION_PA_AB = -0.9999;

struct VertexConnections final
{
        struct Facet
        {
                int facet_index;
                int facet_vertex_index;
                Facet(int facet_index, int facet_vertex_index)
                        : facet_index(facet_index), facet_vertex_index(facet_vertex_index)
                {
                }
        };
        std::vector<int> objects;
        std::vector<Facet> facets;
};

// Definition 4.1 (Poles).
template <std::size_t N>
Vector<N, double> voronoi_positive_norm(
        const Vector<N, double>& vertex,
        const std::vector<DelaunayObject<N>>& delaunay_objects,
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const VertexConnections& vertex_connections)
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

        Vector<N, double> positive_norm;

        if (unbounded)
        {
                Vector<N, double> sum(0);
                for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
                {
                        if (delaunay_facets[vertex_facet.facet_index].one_sided())
                        {
                                sum += delaunay_facets[vertex_facet.facet_index].ortho();
                        }
                }
                positive_norm = sum.normalized();
        }
        else
        {
                double max_distance = limits<double>::lowest();
                Vector<N, double> max_vector(0);
                for (int object_index : vertex_connections.objects)
                {
                        Vector<N, double> voronoi_vertex = delaunay_objects[object_index].voronoi_vertex();
                        Vector<N, double> vp = voronoi_vertex - vertex;
                        double distance = vp.norm_squared();
                        if (distance > max_distance)
                        {
                                max_distance = distance;
                                max_vector = vp;
                        }
                }
                positive_norm = max_vector.normalized();
        }

        if (!is_finite(positive_norm))
        {
                error("Positive pole vector not finite");
        }

        return positive_norm;
}

// Definition 4.1 (Poles).
// Definition 5.3 (The radius and the height of a Voronoi cell).
template <std::size_t N>
double voronoi_height(
        const Vector<N, double>& vertex,
        const std::vector<DelaunayObject<N>>& delaunay_objects,
        const Vector<N, double>& positive_pole_norm,
        const std::vector<int>& vertex_objects)
{
        double max_distance = limits<double>::lowest();
        // Vector<N, double> negative_pole(0);
        bool found = false;

        for (int object_index : vertex_objects)
        {
                Vector<N, double> voronoi_vertex = delaunay_objects[object_index].voronoi_vertex();
                Vector<N, double> vp = voronoi_vertex - vertex;

                if (dot(vp, positive_pole_norm) >= 0)
                {
                        continue;
                }

                double distance = vp.norm_squared();
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

template <std::size_t N>
double voronoi_edge_radius(
        const std::vector<DelaunayObject<N>>& delaunay_objects,
        const DelaunayFacet<N>& facet,
        const Vector<N, double>& positive_pole,
        const Vector<N, double>& pa,
        double pa_length,
        double pb_length,
        double cos_n_a,
        double cos_n_b)
{
        if (facet.one_sided() && cocone_inside_or_equal(cos_n_b))
        {
                return limits<double>::max();
        }

        if (!facet.one_sided() && cocone_inside_or_equal(cos_n_a, cos_n_b))
        {
                return std::max(pa_length, pb_length);
        }

        // here Voronoi vertices are not equal (if equal then they are inside cocone),
        // so it is possible to take a non-zero vector from a to b.
        Vector<N, double> a_to_b = facet.one_sided() ? facet.ortho()
                                                     : (delaunay_objects[facet.delaunay(1)].voronoi_vertex()
                                                        - delaunay_objects[facet.delaunay(0)].voronoi_vertex());

        std::optional<double> max_distance = intersect_cocone_max_distance(positive_pole, pa, a_to_b);
        if (!max_distance)
        {
                // if PA is close to positive pole
                if (std::abs(cos_n_a) > LIMIT_COSINE_FOR_INTERSECTION_PA_POLE)
                {
                        double a_to_b_length = facet.one_sided() ? 1.0 : a_to_b.norm();
                        double cos_pa_ab = dot(pa, a_to_b) / (pa_length * a_to_b_length);

                        // if PA and AB are in opposite directions
                        if (cos_pa_ab < LIMIT_COSINE_FOR_INTERSECTION_PA_AB)
                        {
                                // close to vertex
                                max_distance = 0;
                        }
                        else
                        {
                                error("Cocone intersection not found, PA is close to positive pole");
                        }
                }
                else
                {
                        error("Cocone intersection not found, PA is far from positive pole");
                }
        }

        if (!is_finite(*max_distance))
        {
                error("Cocone intersection distance is not finite");
        }

        if (cocone_inside_or_equal(cos_n_a))
        {
                return std::max(pa_length, *max_distance);
        }

        return *max_distance;
}

// Definition 5.3 (The radius and the height of a Voronoi cell).
template <std::size_t N>
void cocone_facets_and_voronoi_radius(
        const Vector<N, double>& vertex,
        const std::vector<DelaunayObject<N>>& delaunay_objects,
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const Vector<N, double>& positive_pole,
        const VertexConnections& vertex_connections,
        bool find_radius,
        std::vector<ManifoldFacet<N>>* facet_data,
        double* radius)
{
        ASSERT(delaunay_facets.size() == facet_data->size());

        *radius = 0;

        for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
        {
                const DelaunayFacet<N>& facet = delaunay_facets[vertex_facet.facet_index];

                Vector<N, double> pa = delaunay_objects[facet.delaunay(0)].voronoi_vertex() - vertex;
                double pa_length = pa.norm();
                double cos_n_a = dot(positive_pole, pa) / pa_length;

                double pb_length;
                double cos_n_b;
                if (facet.one_sided())
                {
                        pb_length = 0;
                        cos_n_b = dot(positive_pole, facet.ortho());
                }
                else
                {
                        Vector<N, double> pb = delaunay_objects[facet.delaunay(1)].voronoi_vertex() - vertex;
                        pb_length = pb.norm();
                        cos_n_b = dot(positive_pole, pb) / pb_length;
                }

                if (!voronoi_edge_intersects_cocone(cos_n_a, cos_n_b))
                {
                        continue;
                }

                // The facet is marked as a cocone facet only if the Voronoi edge
                // intersects cocones of all N vertices of the facet.
                // The intersection is found for this facet vertex.
                (*facet_data)[vertex_facet.facet_index].cocone_vertex[vertex_facet.facet_vertex_index] = true;

                if (find_radius && *radius != limits<double>::max())
                {
                        double edge_radius = voronoi_edge_radius(
                                delaunay_objects, facet, positive_pole, pa, pa_length, pb_length, cos_n_a, cos_n_b);

                        *radius = std::max(*radius, edge_radius);
                }
        }

        ASSERT(!find_radius || (*radius > 0 && *radius <= limits<double>::max()));
}

// 5.1.2 Flat Sample Points.
// The set of points in P whose Voronoi cells intersect
// the cocone of p are called the cocone neighbors of p.
template <std::size_t N>
void cocone_neighbors(
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const std::vector<ManifoldFacet<N>>& facet_data,
        const std::vector<VertexConnections>& vertex_connections,
        std::vector<ManifoldVertex<N>>* vertex_data)
{
        ASSERT(delaunay_facets.size() == facet_data.size());
        ASSERT(vertex_connections.size() == vertex_data->size());

        int vertex_count = vertex_connections.size();

        for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
                for (const VertexConnections::Facet& vertex_facet : vertex_connections[vertex_index].facets)
                {
                        int facet_index = vertex_facet.facet_index;
                        unsigned skip_v = vertex_facet.facet_vertex_index;

                        for (unsigned v = 0; v < N; ++v)
                        {
                                if (v == skip_v)
                                {
                                        ASSERT(delaunay_facets[facet_index].vertices()[v] == vertex_index);
                                        continue;
                                }

                                if (facet_data[facet_index].cocone_vertex[v])
                                {
                                        (*vertex_data)[vertex_index].cocone_neighbors.push_back(
                                                delaunay_facets[facet_index].vertices()[v]);
                                }
                        }
                }

                sort_and_unique(&(*vertex_data)[vertex_index].cocone_neighbors);
        }
}

template <std::size_t N>
void vertex_connections(
        int vertex_count,
        const std::vector<DelaunayObject<N>>& objects,
        const std::vector<DelaunayFacet<N>>& facets,
        std::vector<VertexConnections>* conn)
{
        conn->clear();
        conn->resize(vertex_count);

        for (unsigned facet = 0; facet < facets.size(); ++facet)
        {
                int local_index = -1;
                for (int vertex : facets[facet].vertices())
                {
                        (*conn)[vertex].facets.emplace_back(facet, ++local_index);
                }
        }

        for (unsigned object = 0; object < objects.size(); ++object)
        {
                for (int vertex : objects[object].vertices())
                {
                        (*conn)[vertex].objects.emplace_back(object);
                }
        }
}
}

template <std::size_t N>
void vertex_and_facet_data(
        bool find_all_vertex_data,
        const std::vector<Vector<N, double>>& points,
        const std::vector<DelaunayObject<N>>& objects,
        const std::vector<DelaunayFacet<N>>& facets,
        std::vector<ManifoldVertex<N>>* vertex_data,
        std::vector<ManifoldFacet<N>>* facet_data)
{
        std::vector<VertexConnections> connections;

        vertex_connections(points.size(), objects, facets, &connections);

        vertex_data->clear();
        vertex_data->reserve(points.size());

        facet_data->clear();
        facet_data->resize(facets.size());

        for (unsigned v = 0; v < points.size(); ++v)
        {
                if (connections[v].facets.empty() && connections[v].objects.empty())
                {
                        // No all points are Delaunay vertices.
                        // Integer convex hull algorithm can skip some points.
                        vertex_data->emplace_back(Vector<N, double>(0), 0, 0);
                        continue;
                }

                ASSERT(!connections[v].facets.empty() && !connections[v].objects.empty());

                Vector<N, double> positive_norm = voronoi_positive_norm(points[v], objects, facets, connections[v]);

                double radius;

                if (!find_all_vertex_data)
                {
                        cocone_facets_and_voronoi_radius(
                                points[v], objects, facets, positive_norm, connections[v], false /*find_radius*/,
                                facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, 0, 0);
                }
                else
                {
                        double height = voronoi_height(points[v], objects, positive_norm, connections[v].objects);

                        cocone_facets_and_voronoi_radius(
                                points[v], objects, facets, positive_norm, connections[v], true /*find_radius*/,
                                facet_data, &radius);

                        vertex_data->emplace_back(positive_norm, height, radius);
                }
        }

        if (find_all_vertex_data)
        {
                cocone_neighbors(facets, *facet_data, connections, vertex_data);
        }

        ASSERT(vertex_data->size() == points.size());
}

#define VERTEX_AND_FACET_DATA_INSTANTIATION(N)                                                          \
        template void vertex_and_facet_data(                                                            \
                bool, const std::vector<Vector<(N), double>>&, const std::vector<DelaunayObject<(N)>>&, \
                const std::vector<DelaunayFacet<(N)>>&, std::vector<ManifoldVertex<(N)>>*,              \
                std::vector<ManifoldFacet<(N)>>*);

VERTEX_AND_FACET_DATA_INSTANTIATION(2)
VERTEX_AND_FACET_DATA_INSTANTIATION(3)
VERTEX_AND_FACET_DATA_INSTANTIATION(4)
VERTEX_AND_FACET_DATA_INSTANTIATION(5)
}
