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
struct Vertex
{
        int new_index;
        std::vector<Vector<N, double>> weighted_normals;
        std::unordered_set<int> vicinity;
};

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        const std::vector<Vector<N, double>> points_double = to_vector<double>(points);

        std::unordered_map<int, Vertex<N>> vertices;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                Vector<N, double> normal = facet_normal(points_double, facet);
                for (unsigned i = 0; i < N; ++i)
                {
                        auto [iter, inserted] = vertices.try_emplace(facet[i]);
                        if (inserted)
                        {
                                iter->second.new_index = idx++;
                        }

                        double normat_weight = facet_normat_weight_at_vertex(points_double, facet, i);
                        iter->second.weighted_normals.push_back(normat_weight * normal);

                        for (unsigned v = 0; v < i; ++v)
                        {
                                iter->second.vicinity.insert(facet[v]);
                        }
                        for (unsigned v = i + 1; v < N; ++v)
                        {
                                iter->second.vicinity.insert(facet[v]);
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());
        mesh->normals.resize(vertices.size());

        std::vector<Vector<N, double>> vicinity;
        for (const auto& [old_index, vertex] : vertices)
        {
                vicinity.clear();
                vicinity.reserve(1 + vertex.vicinity.size());
                vicinity.push_back(points_double[old_index]);
                for (int v : vertex.vicinity)
                {
                        vicinity.push_back(points_double[v]);
                }
                if (size_t count = std::unordered_set<Vector<N, double>>(vicinity.cbegin(), vicinity.cend()).size();
                    count < N)
                {
                        error("Vertex has " + to_string(count) + " vertices in its vicinity, required minimum is "
                              + to_string(N) + " vertices");
                }

                Vector<N, double> normal = numerical::point_normal(vicinity);

                mesh->vertices[vertex.new_index] = points[old_index];
                mesh->normals[vertex.new_index] = to_vector<float>(average_of_normals(normal, vertex.weighted_normals));
        }

        mesh->facets.reserve(facets.size());

        for (const std::array<int, N>& facet : facets)
        {
                typename Mesh<N>::Facet mesh_facet;

                mesh_facet.material = -1;
                mesh_facet.has_texcoord = false;
                mesh_facet.has_normal = true;

                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = vertices.find(facet[i]);
                        ASSERT(iter != vertices.cend());
                        mesh_facet.vertices[i] = iter->second.new_index;
                        mesh_facet.normals[i] = mesh_facet.vertices[i];
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
        return create_mesh(points, facets);
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
