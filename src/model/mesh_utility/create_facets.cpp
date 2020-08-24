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
#include <src/numerical/orthogonal.h>

#include <unordered_map>
#include <unordered_set>

namespace mesh
{
namespace
{
template <size_t N>
Vector<N, double> face_normal(const std::vector<Vector<N, float>>& points, const std::array<int, N>& face)
{
        return ortho_nn<N, float, double>(points, face).normalized();
}

template <size_t N, typename T>
Vector<N, T> average_normal(const Vector<N, T>& original_normal, const std::vector<Vector<N, T>>& normals)
{
        Vector<N, T> sum(0);
        for (const Vector<N, T>& n : normals)
        {
                sum += (dot(n, original_normal) >= 0) ? n : -n;
        }
        return sum.normalized();
}

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<Vector<N, double>>& point_normals,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        struct Vertex
        {
                int new_index;
                std::vector<Vector<N, double>> normals;
        };
        std::unordered_map<int, Vertex> vertices;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                Vector<N, double> normal = face_normal(points, facet);
                for (int v : facet)
                {
                        auto [iter, inserted] = vertices.try_emplace(v);
                        iter->second.normals.push_back(normal);
                        if (inserted)
                        {
                                iter->second.new_index = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertices.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertices.size());
        mesh->normals.resize(vertices.size());

        for (const auto& [old_index, vertex] : vertices)
        {
                mesh->vertices[vertex.new_index] = points[old_index];
                mesh->normals[vertex.new_index] =
                        to_vector<float>(average_normal(point_normals[old_index], vertex.normals));
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

template <size_t N>
std::unique_ptr<Mesh<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        std::unordered_map<int, int> vertex_map;

        int idx = 0;
        for (const std::array<int, N>& facet : facets)
        {
                for (int v : facet)
                {
                        auto [iter, inserted] = vertex_map.try_emplace(v);
                        if (inserted)
                        {
                                iter->second = idx++;
                        }
                }
        }
        ASSERT(idx == static_cast<int>(vertex_map.size()));

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        mesh->vertices.resize(vertex_map.size());

        for (const auto& [old_index, new_index] : vertex_map)
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
                        auto iter = vertex_map.find(facet[i]);
                        ASSERT(iter != vertex_map.cend());
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
        const std::vector<Vector<N, double>>& point_normals,
        const std::vector<std::array<int, N>>& facets)

{
        ASSERT(points.size() == point_normals.size());

        return create_mesh(points, point_normals, facets);
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
        const std::vector<Vector<3, double>>& point_normals,
        const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<Mesh<4>> create_mesh_for_facets(
        const std::vector<Vector<4, float>>& points,
        const std::vector<Vector<4, double>>& point_normals,
        const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<Mesh<5>> create_mesh_for_facets(
        const std::vector<Vector<5, float>>& points,
        const std::vector<Vector<5, double>>& point_normals,
        const std::vector<std::array<int, 5>>& facets);
template std::unique_ptr<Mesh<6>> create_mesh_for_facets(
        const std::vector<Vector<6, float>>& points,
        const std::vector<Vector<6, double>>& point_normals,
        const std::vector<std::array<int, 6>>& facets);

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
