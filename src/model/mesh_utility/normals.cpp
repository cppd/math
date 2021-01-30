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
 Jakob Andreas Bærentzen, Jens Gravesen, François Anton, Henrik Aanæs.
 Guide to Computational Geometry Processing. Foundations, Algorithms, and Methods.
 Springer-Verlag London, 2012.

 8.1 Estimating the Surface Normal.
*/

#include "normals.h"

#include <src/com/alg.h>
#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/geometry/shapes/sphere_surface.h>
#include <src/numerical/normal.h>
#include <src/numerical/orthogonal.h>

namespace ns::mesh
{
namespace
{
template <std::size_t N, typename T>
T facet_normat_weight_at_vertex(
        const std::vector<Vector<N, T>>& points,
        const std::array<int, N>& facet,
        int facet_vertex_index)
{
        if constexpr (N >= 5)
        {
                return 1;
        }

        if constexpr (N == 4 || N == 3)
        {
                ASSERT(facet_vertex_index >= 0 && facet_vertex_index < static_cast<int>(N));

                std::array<Vector<N, T>, N - 1> vectors;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        int index = (facet_vertex_index + 1 + i) % N;
                        vectors[i] = points[facet[index]] - points[facet[facet_vertex_index]];
                }

                return geometry::sphere_simplex_area(vectors);
        }
}

template <std::size_t N, typename T>
Vector<N, T> average_of_normals(const Vector<N, T>& normal, const std::vector<Vector<N, T>>& normals)
{
        Vector<N, T> sum(0);
        for (const Vector<N, T>& n : normals)
        {
                if (dot(n, normal) >= 0)
                {
                        sum += n;
                }
                else
                {
                        sum -= n;
                }
        }
        return sum.normalized();
}

struct VertexFacet
{
        int facet_index;
        unsigned facet_vertex; // [0, N)
};

template <std::size_t N, typename T>
Vector<N, T> compute_normal(
        const std::vector<Vector<N, T>>& vertices,
        const std::vector<Vector<N, T>>& facet_normals,
        const std::vector<typename Mesh<N>::Facet>& mesh_facets,
        std::size_t vertex_index,
        const std::vector<VertexFacet>& vertex_facets)
{
        thread_local std::vector<int> vicinity_int;
        thread_local std::vector<Vector<N, T>> vicinity;
        thread_local std::vector<Vector<N, T>> weighted_normals;

        vicinity_int.clear();
        vicinity.clear();
        weighted_normals.clear();

        for (const VertexFacet& f : vertex_facets)
        {
                const std::array<int, N>& facet_vertices = mesh_facets[f.facet_index].vertices;
                T weight = facet_normat_weight_at_vertex(vertices, facet_vertices, f.facet_vertex);
                weighted_normals.push_back(weight * facet_normals[f.facet_index]);

                for (unsigned fv = 0; fv < f.facet_vertex; ++fv)
                {
                        vicinity_int.push_back(facet_vertices[fv]);
                }
                for (unsigned fv = f.facet_vertex + 1; fv < N; ++fv)
                {
                        vicinity_int.push_back(facet_vertices[fv]);
                }
        }

        vicinity_int.push_back(vertex_index);
        sort_and_unique(&vicinity_int);
        if (vicinity_int.size() < N)
        {
                error("Vertex has " + to_string(vicinity_int.size()) + " vertices in its vicinity, required minimum is "
                      + to_string(N) + " vertices");
        }

        for (int vi : vicinity_int)
        {
                vicinity.push_back(vertices[vi]);
        }

        Vector<N, T> point_normal = numerical::point_normal(vicinity);

        return average_of_normals(point_normal, weighted_normals);
}
}

template <std::size_t N>
void compute_normals(Mesh<N>* mesh)
{
        using ComputeType = double;

        if (mesh->facets.empty())
        {
                mesh->normals.clear();
                return;
        }
        mesh->normals.resize(mesh->vertices.size());

        const std::vector<Vector<N, ComputeType>> vertices = to_vector<ComputeType>(mesh->vertices);

        std::vector<Vector<N, ComputeType>> facet_normals(mesh->facets.size());
        std::vector<std::vector<VertexFacet>> vertex_facets(mesh->vertices.size());
        for (std::size_t f = 0; f < mesh->facets.size(); ++f)
        {
                const typename Mesh<N>::Facet& facet = mesh->facets[f];

                facet_normals[f] = numerical::ortho_nn(vertices, facet.vertices).normalized();

                for (unsigned i = 0; i < N; ++i)
                {
                        int vertex = facet.vertices[i];
                        ASSERT(vertex < static_cast<int>(vertex_facets.size()));
                        VertexFacet& vertex_facet = vertex_facets[vertex].emplace_back();
                        vertex_facet.facet_index = f;
                        vertex_facet.facet_vertex = i;
                }
        }

        for (std::size_t v = 0; v < vertex_facets.size(); ++v)
        {
                mesh->normals[v] =
                        to_vector<float>(compute_normal(vertices, facet_normals, mesh->facets, v, vertex_facets[v]));
        }

        for (typename Mesh<N>::Facet& facet : mesh->facets)
        {
                facet.has_normal = true;
                facet.normals = facet.vertices;
        }
}

template void compute_normals(Mesh<3>* mesh);
template void compute_normals(Mesh<4>* mesh);
template void compute_normals(Mesh<5>* mesh);
template void compute_normals(Mesh<6>* mesh);
}
