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
std::unique_ptr<MeshModel<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<Vector<N, double>>& point_normals,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        std::unordered_map<int, std::vector<Vector<N, double>>> vertices;
        std::unordered_map<int, int> index_map;

        for (const std::array<int, N>& facet : facets)
        {
                Vector<N, double> normal = face_normal(points, facet);
                for (int v : facet)
                {
                        vertices[v].push_back(normal);
                }
        }

        std::unique_ptr<MeshModel<N>> mesh = std::make_unique<MeshModel<N>>();

        mesh->vertices.resize(vertices.size());
        mesh->normals.resize(vertices.size());

        int idx = 0;
        for (const auto& [vertex, normals] : vertices)
        {
                index_map[vertex] = idx;

                mesh->vertices[idx] = points[vertex];
                mesh->normals[idx] = to_vector<float>(average_normal(point_normals[vertex], normals));

                ++idx;
        }

        mesh->facets.reserve(facets.size());

        for (const std::array<int, N>& facet : facets)
        {
                typename MeshModel<N>::Facet mesh_facet;

                mesh_facet.material = -1;
                mesh_facet.has_texcoord = false;
                mesh_facet.has_normal = true;

                for (unsigned i = 0; i < N; ++i)
                {
                        mesh_facet.vertices[i] = index_map[facet[i]];
                        mesh_facet.normals[i] = mesh_facet.vertices[i];
                        mesh_facet.texcoords[i] = -1;
                }

                mesh->facets.push_back(std::move(mesh_facet));
        }

        set_center_and_length(mesh.get());

        return mesh;
}

template <size_t N>
std::unique_ptr<MeshModel<N>> create_mesh(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        if (facets.empty())
        {
                error("No facets for facet object");
        }

        std::unordered_set<int> vertices;
        std::unordered_map<int, int> index_map;

        for (const std::array<int, N>& facet : facets)
        {
                for (int v : facet)
                {
                        vertices.insert(v);
                }
        }

        std::unique_ptr<MeshModel<N>> mesh = std::make_unique<MeshModel<N>>();

        mesh->vertices.resize(vertices.size());

        int idx = 0;
        for (auto v : vertices)
        {
                index_map[v] = idx;
                mesh->vertices[idx] = points[v];
                ++idx;
        }

        mesh->facets.reserve(facets.size());

        for (const std::array<int, N>& facet : facets)
        {
                typename MeshModel<N>::Facet mesh_facet;

                mesh_facet.material = -1;
                mesh_facet.has_texcoord = false;
                mesh_facet.has_normal = false;

                for (unsigned i = 0; i < N; ++i)
                {
                        mesh_facet.vertices[i] = index_map[facet[i]];
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
std::unique_ptr<MeshModel<N>> create_mesh_for_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<Vector<N, double>>& point_normals,
        const std::vector<std::array<int, N>>& facets)

{
        ASSERT(points.size() == point_normals.size());

        return create_mesh(points, point_normals, facets);
}

template <size_t N>
std::unique_ptr<MeshModel<N>> create_mesh_for_facets(
        const std::vector<Vector<N, float>>& points,
        const std::vector<std::array<int, N>>& facets)
{
        return create_mesh(points, facets);
}

template std::unique_ptr<MeshModel<3>> create_mesh_for_facets(
        const std::vector<Vector<3, float>>& points,
        const std::vector<Vector<3, double>>& point_normals,
        const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<MeshModel<4>> create_mesh_for_facets(
        const std::vector<Vector<4, float>>& points,
        const std::vector<Vector<4, double>>& point_normals,
        const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<MeshModel<5>> create_mesh_for_facets(
        const std::vector<Vector<5, float>>& points,
        const std::vector<Vector<5, double>>& point_normals,
        const std::vector<std::array<int, 5>>& facets);
template std::unique_ptr<MeshModel<6>> create_mesh_for_facets(
        const std::vector<Vector<6, float>>& points,
        const std::vector<Vector<6, double>>& point_normals,
        const std::vector<std::array<int, 6>>& facets);

template std::unique_ptr<MeshModel<3>> create_mesh_for_facets(
        const std::vector<Vector<3, float>>& points,
        const std::vector<std::array<int, 3>>& facets);
template std::unique_ptr<MeshModel<4>> create_mesh_for_facets(
        const std::vector<Vector<4, float>>& points,
        const std::vector<std::array<int, 4>>& facets);
template std::unique_ptr<MeshModel<5>> create_mesh_for_facets(
        const std::vector<Vector<5, float>>& points,
        const std::vector<std::array<int, 5>>& facets);
template std::unique_ptr<MeshModel<6>> create_mesh_for_facets(
        const std::vector<Vector<6, float>>& points,
        const std::vector<std::array<int, 6>>& facets);
