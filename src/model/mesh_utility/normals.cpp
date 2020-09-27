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
 Jakob Andreas Bærentzen, Jens Gravesen, François Anton, Henrik Aanæs.
 Guide to Computational Geometry Processing. Foundations, Algorithms, and Methods.
 Springer-Verlag London, 2012.

 8.1 Estimating the Surface Normal.
*/

#include "normals.h"

#include <src/com/alg.h>
#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/numerical/normal.h>
#include <src/numerical/orthogonal.h>

namespace mesh
{
namespace
{
template <size_t N>
Vector<N, double> facet_normal(const std::vector<Vector<N, double>>& points, const std::array<int, N>& facet)
{
        return ortho_nn(points, facet).normalized();
}

double spherical_triangle_area(const std::array<Vector<4, double>, 3>& vectors, const Vector<4, double>& normal)
{
        std::array<Vector<4, double>, 3> v;
        v[2] = normal;

        v[0] = vectors[0];
        v[1] = vectors[1];
        Vector<4, double> edge_01_normal = ortho_nn(v).normalized();

        v[0] = vectors[1];
        v[1] = vectors[2];
        Vector<4, double> edge_12_normal = ortho_nn(v).normalized();

        v[0] = vectors[2];
        v[1] = vectors[0];
        Vector<4, double> edge_20_normal = ortho_nn(v).normalized();

        double dihedral_cosine_0 = -dot(edge_01_normal, edge_20_normal);
        double dihedral_cosine_1 = -dot(edge_01_normal, edge_12_normal);
        double dihedral_cosine_2 = -dot(edge_20_normal, edge_12_normal);
        if (!is_finite(dihedral_cosine_0) || !is_finite(dihedral_cosine_1) || !is_finite(dihedral_cosine_2))
        {
                return 0;
        }

        double dihedral_0 = std::acos(std::clamp(dihedral_cosine_0, -1.0, 1.0));
        double dihedral_1 = std::acos(std::clamp(dihedral_cosine_1, -1.0, 1.0));
        double dihedral_2 = std::acos(std::clamp(dihedral_cosine_2, -1.0, 1.0));

        double area = dihedral_0 + dihedral_1 + dihedral_2 - PI<double>;

        return std::max(0.0, area);
}

template <size_t N>
double facet_normat_weight_at_vertex(
        const std::vector<Vector<N, double>>& points,
        const std::array<int, N>& facet,
        int facet_vertex_index,
        const Vector<N, double>& facet_normal)
{
        if constexpr (N >= 5)
        {
                return 1.0;
        }

        if constexpr (N == 4 || N == 3)
        {
                ASSERT(facet_vertex_index >= 0 && facet_vertex_index < static_cast<int>(N));

                std::array<Vector<N, double>, N - 1> vectors;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        int index = (facet_vertex_index + 1 + i) % N;
                        Vector<N, double> v = points[facet[index]] - points[facet[facet_vertex_index]];
                        double norm = v.norm();
                        if (norm == 0)
                        {
                                return 0;
                        }
                        vectors[i] = v / norm;
                }

                if constexpr (N == 4)
                {
                        return spherical_triangle_area(vectors, facet_normal);
                }
                if constexpr (N == 3)
                {
                        double cosine = dot(vectors[0], vectors[1]);
                        return std::acos(std::clamp(cosine, -1.0, 1.0));
                }
        }
}

template <size_t N, typename T>
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

template <size_t N>
Vector<N, double> compute_normal(
        const std::vector<Vector<N, double>>& vertices,
        const std::vector<Vector<N, double>>& facet_normals,
        const std::vector<typename Mesh<N>::Facet>& mesh_facets,
        size_t vertex_index,
        const std::vector<VertexFacet>& vertex_facets)
{
        thread_local std::vector<int> vicinity_int;
        thread_local std::vector<Vector<N, double>> vicinity;
        thread_local std::vector<Vector<N, double>> weighted_normals;

        vicinity_int.clear();
        vicinity.clear();
        weighted_normals.clear();

        for (const VertexFacet& f : vertex_facets)
        {
                const std::array<int, N>& facet_vertices = mesh_facets[f.facet_index].vertices;
                double weight = facet_normat_weight_at_vertex(
                        vertices, facet_vertices, f.facet_vertex, facet_normals[f.facet_index]);
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

        Vector<N, double> point_normal = numerical::point_normal(vicinity);

        return average_of_normals(point_normal, weighted_normals);
}
}

template <size_t N>
void compute_normals(Mesh<N>* mesh)
{
        if (mesh->facets.empty())
        {
                mesh->normals.clear();
                return;
        }
        mesh->normals.resize(mesh->vertices.size());

        const std::vector<Vector<N, double>> vertices = to_vector<double>(mesh->vertices);

        std::vector<Vector<N, double>> facet_normals(mesh->facets.size());
        std::vector<std::vector<VertexFacet>> vertex_facets(mesh->vertices.size());
        for (size_t f = 0; f < mesh->facets.size(); ++f)
        {
                const typename Mesh<N>::Facet& facet = mesh->facets[f];

                facet_normals[f] = facet_normal(vertices, facet.vertices);

                for (unsigned i = 0; i < N; ++i)
                {
                        int vertex = facet.vertices[i];
                        ASSERT(vertex < static_cast<int>(vertex_facets.size()));
                        VertexFacet& vertex_facet = vertex_facets[vertex].emplace_back();
                        vertex_facet.facet_index = f;
                        vertex_facet.facet_vertex = i;
                }
        }

        for (size_t v = 0; v < vertex_facets.size(); ++v)
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
