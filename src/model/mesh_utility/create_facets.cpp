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

#include "create_facets.h"

#include "position.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/numerical/normal.h>
#include <src/numerical/orthogonal.h>

#include <unordered_map>
#include <unordered_set>

/*
 Jakob Andreas Bærentzen, Jens Gravesen, François Anton, Henrik Aanæs.
 Guide to Computational Geometry Processing. Foundations, Algorithms, and Methods.
 Springer-Verlag London, 2012.

 8.1 Estimating the Surface Normal.
*/

namespace mesh
{
namespace
{
template <size_t N>
Vector<N, double> facet_normal(const std::vector<Vector<N, double>>& points, const std::array<int, N>& facet)
{
        return ortho_nn(points, facet).normalized();
}

template <size_t N>
double facet_normat_weight_at_vertex(
        const std::vector<Vector<N, double>>& points,
        const std::array<int, N>& facet,
        int facet_vertex_index)
{
        if constexpr (N != 3)
        {
                return 1.0;
        }
        else
        {
                ASSERT(facet_vertex_index >= 0 && facet_vertex_index < 3);
                int n1 = (facet_vertex_index + 1) % 3;
                int n2 = (facet_vertex_index + 2) % 3;
                vec3 v1 = points[facet[n1]] - points[facet[facet_vertex_index]];
                vec3 v2 = points[facet[n2]] - points[facet[facet_vertex_index]];
                double norm1 = v1.norm();
                if (norm1 == 0)
                {
                        return 0;
                }
                double norm2 = v2.norm();
                if (norm2 == 0)
                {
                        return 0;
                }
                double cosine = dot(v1 / norm1, v2 / norm2);
                return std::acos(std::clamp(cosine, -1.0, 1.0));
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

template <size_t N>
void compute_mesh_normals(Mesh<N>* mesh)
{
        if (mesh->facets.empty())
        {
                mesh->normals.clear();
                return;
        }
        mesh->normals.resize(mesh->vertices.size());

        const std::vector<Vector<N, double>> vertices = to_vector<double>(mesh->vertices);

        std::vector<Vector<N, double>> facet_normals(mesh->facets.size());

        struct VertexFacet
        {
                int facet_index;
                unsigned facet_vertex; // [0, N)
        };
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

        std::vector<int> vicinity_int;
        std::vector<Vector<N, double>> vicinity;
        std::vector<Vector<N, double>> weighted_normals;
        for (size_t v = 0; v < vertex_facets.size(); ++v)
        {
                vicinity_int.clear();
                vicinity.clear();
                weighted_normals.clear();

                for (const VertexFacet& f : vertex_facets[v])
                {
                        const std::array<int, N>& facet_vertices = mesh->facets[f.facet_index].vertices;
                        double weight = facet_normat_weight_at_vertex(vertices, facet_vertices, f.facet_vertex);
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
                vicinity_int.push_back(v);
                sort_and_unique(&vicinity_int);

                for (int vi : vicinity_int)
                {
                        vicinity.push_back(vertices[vi]);
                }

                if (size_t count = std::unordered_set<Vector<N, double>>(vicinity.cbegin(), vicinity.cend()).size();
                    count < N)
                {
                        error("Vertex has " + to_string(count) + " vertices in its vicinity, required minimum is "
                              + to_string(N) + " vertices");
                }

                Vector<N, double> point_normal = numerical::point_normal(vicinity);

                mesh->normals[v] = to_vector<float>(average_of_normals(point_normal, weighted_normals));
        }

        for (typename Mesh<N>::Facet& facet : mesh->facets)
        {
                facet.has_normal = true;
                facet.normals = facet.vertices;
        }
}

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        std::unordered_map<int, int> vertices;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                for (int vertex_index : facet)
                {
                        auto [iter, inserted] = vertices.try_emplace(vertex_index);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());
        for (const auto& [old_index, new_index] : vertices)
        {
                mesh->vertices[new_index] = points[old_index];
        }

        mesh->facets.reserve(facets.size());
        for (const std::array<int, N>& facet : facets)
        {
                typename Mesh<N>::Facet mesh_facet;

                mesh_facet.material = -1;
                mesh_facet.has_texcoord = false;
                mesh_facet.has_normal = false;

                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = vertices.find(facet[i]);
                        ASSERT(iter != vertices.cend());
                        mesh_facet.vertices[i] = iter->second;
                        mesh_facet.normals[i] = -1;
                        mesh_facet.texcoords[i] = -1;
                }

                mesh->facets.push_back(std::move(mesh_facet));
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh_for_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        std::unique_ptr<Mesh<N>> mesh = create_mesh(points, facets);
        compute_mesh_normals(mesh.get());
        return mesh;
}

template std::unique_ptr<Mesh<3>> create_mesh_for_facets(
        const std::vector<Vector<3, float>>& points,
        const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<Mesh<4>> create_mesh_for_facets(
        const std::vector<Vector<4, float>>& points,
        const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<Mesh<5>> create_mesh_for_facets(
        const std::vector<Vector<5, float>>& points,
        const std::vector<std::array<int, 5>>& facets);
template std::unique_ptr<Mesh<6>> create_mesh_for_facets(
        const std::vector<Vector<6, float>>& points,
        const std::vector<std::array<int, 6>>& facets);
}
