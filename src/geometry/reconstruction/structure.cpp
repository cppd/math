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
*/

#include "structure.h"

#include "functions.h"

#include "../core/delaunay.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace ns::geometry::reconstruction
{
namespace
{
constexpr double LIMIT_COSINE_FOR_INTERSECTION_PA_POLE = 0.99;
constexpr double LIMIT_COSINE_FOR_INTERSECTION_PA_AB = -0.9999;

constexpr double MAX_VORONOI_EDGE_RADIUS = Limits<double>::max();

struct VertexConnections final
{
        struct Facet final
        {
                int facet_index;
                int facet_vertex_index;

                Facet(const int facet_index, const int facet_vertex_index)
                        : facet_index(facet_index),
                          facet_vertex_index(facet_vertex_index)
                {
                }
        };

        std::vector<int> objects;
        std::vector<Facet> facets;
};

template <std::size_t N>
bool is_unbounded(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const VertexConnections& vertex_connections)
{
        for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
        {
                if (delaunay_facets[vertex_facet.facet_index].one_sided())
                {
                        return true;
                }
        }
        return false;
}

// Definition 4.1 (Poles).
template <std::size_t N>
Vector<N, double> voronoi_positive_norm(
        const Vector<N, double>& vertex,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const VertexConnections& vertex_connections)
{
        const bool unbounded = is_unbounded(delaunay_facets, vertex_connections);

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
                double max_distance = Limits<double>::lowest();
                Vector<N, double> max_vector(0);
                for (const auto object_index : vertex_connections.objects)
                {
                        const Vector<N, double> voronoi_vertex = delaunay_objects[object_index].voronoi_vertex();
                        const Vector<N, double> vp = voronoi_vertex - vertex;
                        const double distance = vp.norm_squared();
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
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const Vector<N, double>& positive_pole_norm,
        const std::vector<int>& vertex_objects)
{
        double max_distance = Limits<double>::lowest();
        // Vector<N, double> negative_pole(0);
        bool found = false;

        for (const int object_index : vertex_objects)
        {
                const Vector<N, double> voronoi_vertex = delaunay_objects[object_index].voronoi_vertex();
                const Vector<N, double> vp = voronoi_vertex - vertex;

                if (dot(vp, positive_pole_norm) >= 0)
                {
                        continue;
                }

                const double distance = vp.norm_squared();
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

        const double len = std::sqrt(max_distance);

        if (!std::isfinite(len))
        {
                error("Negative pole vector not finite");
        }

        return len;
}

template <std::size_t N>
void check_close_to_vertex(
        const core::DelaunayFacet<N>& facet,
        const Vector<N, double>& pa,
        const double pa_length,
        const double cos_n_a,
        const Vector<N, double>& a_to_b)
{
        // if PA is close to positive pole
        if (std::abs(cos_n_a) > LIMIT_COSINE_FOR_INTERSECTION_PA_POLE)
        {
                const double a_to_b_length = facet.one_sided() ? 1 : a_to_b.norm();
                const double cos_pa_ab = dot(pa, a_to_b) / (pa_length * a_to_b_length);

                // if PA and AB are in opposite directions
                if (cos_pa_ab < LIMIT_COSINE_FOR_INTERSECTION_PA_AB)
                {
                        // close to vertex
                        return;
                }

                error("Cocone intersection not found, PA is close to positive pole");
        }

        error("Cocone intersection not found, PA is far from positive pole");
}

template <std::size_t N>
double voronoi_edge_radius(
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const core::DelaunayFacet<N>& facet,
        const Vector<N, double>& positive_pole,
        const Vector<N, double>& pa,
        const double pa_length,
        const double pb_length,
        const double cos_n_a,
        const double cos_n_b)
{
        if (facet.one_sided() && cocone_inside_or_equal(cos_n_b))
        {
                return MAX_VORONOI_EDGE_RADIUS;
        }

        if (!facet.one_sided() && cocone_inside_or_equal(cos_n_a, cos_n_b))
        {
                return std::max(pa_length, pb_length);
        }

        // here Voronoi vertices are not equal (if equal then they are inside cocone),
        // so it is possible to take a non-zero vector from a to b.
        const Vector<N, double> a_to_b =
                facet.one_sided()
                        ? facet.ortho()
                        : (delaunay_objects[facet.delaunay(1)].voronoi_vertex()
                           - delaunay_objects[facet.delaunay(0)].voronoi_vertex());

        std::optional<double> max_distance = intersect_cocone_max_distance(positive_pole, pa, a_to_b);
        if (!max_distance)
        {
                check_close_to_vertex(facet, pa, pa_length, cos_n_a, a_to_b);
                max_distance = 0;
        }

        if (!std::isfinite(*max_distance))
        {
                error("Cocone intersection distance is not finite");
        }

        if (cocone_inside_or_equal(cos_n_a))
        {
                return std::max(pa_length, *max_distance);
        }

        return *max_distance;
}

template <std::size_t N>
struct EdgePoint final
{
        Vector<N, double> v;
        double length;
        double cos;
};

template <std::size_t N>
EdgePoint<N> compute_edge_point(
        const unsigned index,
        const Vector<N, double>& vertex,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const Vector<N, double>& positive_pole,
        const core::DelaunayFacet<N>& facet)
{
        const Vector<N, double> v = delaunay_objects[facet.delaunay(index)].voronoi_vertex() - vertex;
        const double length = v.norm();
        const double cos = dot(positive_pole, v) / length;
        return {.v = v, .length = length, .cos = cos};
}

// Definition 5.3 (The radius and the height of a Voronoi cell).
template <std::size_t N>
double cocone_facets_and_voronoi_radius_impl(
        const Vector<N, double>& vertex,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const Vector<N, double>& positive_pole,
        const VertexConnections& vertex_connections,
        std::vector<ManifoldFacet<N>>* const facet_data,
        const bool find_radius)
{
        ASSERT(delaunay_facets.size() == facet_data->size());

        double radius = 0;

        for (const VertexConnections::Facet& vertex_facet : vertex_connections.facets)
        {
                const core::DelaunayFacet<N>& facet = delaunay_facets[vertex_facet.facet_index];

                const EdgePoint pa = compute_edge_point(0, vertex, delaunay_objects, positive_pole, facet);

                const EdgePoint pb = [&]
                {
                        if (facet.one_sided())
                        {
                                return EdgePoint<N>{.v = {}, .length = 0, .cos = dot(positive_pole, facet.ortho())};
                        }
                        return compute_edge_point(1, vertex, delaunay_objects, positive_pole, facet);
                }();

                if (!voronoi_edge_intersects_cocone(pa.cos, pb.cos))
                {
                        continue;
                }

                // The facet is marked as a cocone facet only if the Voronoi edge
                // intersects cocones of all N vertices of the facet.
                // The intersection is found for this facet vertex.
                (*facet_data)[vertex_facet.facet_index].cocone_vertex[vertex_facet.facet_vertex_index] = true;

                if (find_radius && radius != MAX_VORONOI_EDGE_RADIUS)
                {
                        const double edge_radius = voronoi_edge_radius(
                                delaunay_objects, facet, positive_pole, pa.v, pa.length, pb.length, pa.cos, pb.cos);

                        radius = std::max(radius, edge_radius);
                }
        }

        ASSERT(!find_radius || (radius > 0 && radius <= MAX_VORONOI_EDGE_RADIUS));

        return radius;
}

template <std::size_t N>
double cocone_facets_and_voronoi_radius(
        const Vector<N, double>& vertex,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const Vector<N, double>& positive_pole,
        const VertexConnections& vertex_connections,
        std::vector<ManifoldFacet<N>>* const facet_data)
{
        constexpr bool FIND_RADIUS = true;
        return cocone_facets_and_voronoi_radius_impl(
                vertex, delaunay_objects, delaunay_facets, positive_pole, vertex_connections, facet_data, FIND_RADIUS);
}

template <std::size_t N>
void cocone_facets(
        const Vector<N, double>& vertex,
        const std::vector<core::DelaunayObject<N>>& delaunay_objects,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const Vector<N, double>& positive_pole,
        const VertexConnections& vertex_connections,
        std::vector<ManifoldFacet<N>>* const facet_data)
{
        constexpr bool FIND_RADIUS = false;
        cocone_facets_and_voronoi_radius_impl(
                vertex, delaunay_objects, delaunay_facets, positive_pole, vertex_connections, facet_data, FIND_RADIUS);
}

// 5.1.2 Flat Sample Points.
// The set of points in P whose Voronoi cells intersect
// the cocone of p are called the cocone neighbors of p.
template <std::size_t N>
void cocone_neighbors(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<ManifoldFacet<N>>& facet_data,
        const int vertex_index,
        const VertexConnections::Facet& vertex_facet,
        std::vector<ManifoldVertex<N>>* const vertex_data)
{
        const int facet_index = vertex_facet.facet_index;
        const unsigned skip_index = vertex_facet.facet_vertex_index;

        for (std::size_t i = 0; i < N; ++i)
        {
                if (i == skip_index)
                {
                        ASSERT(delaunay_facets[facet_index].vertices()[i] == vertex_index);
                        continue;
                }

                if (facet_data[facet_index].cocone_vertex[i])
                {
                        (*vertex_data)[vertex_index].cocone_neighbors.push_back(
                                delaunay_facets[facet_index].vertices()[i]);
                }
        }
}

template <std::size_t N>
void cocone_neighbors(
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<ManifoldFacet<N>>& facet_data,
        const std::vector<VertexConnections>& vertex_connections,
        std::vector<ManifoldVertex<N>>* const vertex_data)
{
        ASSERT(delaunay_facets.size() == facet_data.size());
        ASSERT(vertex_connections.size() == vertex_data->size());

        const int vertex_count = vertex_connections.size();
        for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
                for (const VertexConnections::Facet& vertex_facet : vertex_connections[vertex_index].facets)
                {
                        cocone_neighbors(delaunay_facets, facet_data, vertex_index, vertex_facet, vertex_data);
                }
                sort_and_unique(&(*vertex_data)[vertex_index].cocone_neighbors);
        }
}

template <std::size_t N>
std::vector<VertexConnections> vertex_connections(
        const int vertex_count,
        const std::vector<core::DelaunayObject<N>>& objects,
        const std::vector<core::DelaunayFacet<N>>& facets)
{
        std::vector<VertexConnections> connections(vertex_count);

        for (std::size_t facet = 0; facet < facets.size(); ++facet)
        {
                int local_index = -1;
                for (const auto vertex : facets[facet].vertices())
                {
                        ASSERT(vertex < vertex_count);
                        connections[vertex].facets.emplace_back(facet, ++local_index);
                }
        }

        for (std::size_t object = 0; object < objects.size(); ++object)
        {
                for (const auto vertex : objects[object].vertices())
                {
                        ASSERT(vertex < vertex_count);
                        connections[vertex].objects.emplace_back(object);
                }
        }

        return connections;
}
}

template <std::size_t N>
ManifoldData<N> find_manifold_data(
        const bool find_cocone_neighbors,
        const std::vector<Vector<N, double>>& points,
        const std::vector<core::DelaunayObject<N>>& objects,
        const std::vector<core::DelaunayFacet<N>>& facets)
{
        const std::vector<VertexConnections> connections = vertex_connections(points.size(), objects, facets);

        std::vector<ManifoldVertex<N>> vertex_data;
        std::vector<ManifoldFacet<N>> facet_data;
        vertex_data.reserve(points.size());
        facet_data.resize(facets.size());

        for (std::size_t v = 0; v < points.size(); ++v)
        {
                if (connections[v].facets.empty() && connections[v].objects.empty())
                {
                        // No all points are Delaunay vertices.
                        // Integer convex hull algorithm can skip some points.
                        vertex_data.emplace_back(Vector<N, double>(0), 0, 0);
                        continue;
                }

                ASSERT(!connections[v].facets.empty() && !connections[v].objects.empty());

                const Vector<N, double> positive_norm =
                        voronoi_positive_norm(points[v], objects, facets, connections[v]);

                if (!find_cocone_neighbors)
                {
                        cocone_facets(points[v], objects, facets, positive_norm, connections[v], &facet_data);

                        vertex_data.emplace_back(positive_norm, 0, 0);
                }
                else
                {
                        const double height = voronoi_height(points[v], objects, positive_norm, connections[v].objects);

                        const double voronoi_radius = cocone_facets_and_voronoi_radius(
                                points[v], objects, facets, positive_norm, connections[v], &facet_data);

                        vertex_data.emplace_back(positive_norm, height, voronoi_radius);
                }
        }

        if (find_cocone_neighbors)
        {
                cocone_neighbors(facets, facet_data, connections, &vertex_data);
        }

        ASSERT(vertex_data.size() == points.size());

        return {.vertices = std::move(vertex_data), .facets = std::move(facet_data)};
}

#define TEMPLATE(N)                                                                                           \
        template ManifoldData<(N)> find_manifold_data(                                                        \
                bool, const std::vector<Vector<(N), double>>&, const std::vector<core::DelaunayObject<(N)>>&, \
                const std::vector<core::DelaunayFacet<(N)>>&);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
