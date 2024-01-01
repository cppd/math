/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "optimize.h"

#include <src/geometry/spatial/hyperplane.h>
#include <src/geometry/spatial/point_offset.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::shapes::mesh
{
namespace
{
template <std::size_t N, typename T>
[[nodiscard]] bool vertex_inside_clip_plane(
        const Vector<N, T>& vertex,
        const geometry::spatial::Hyperplane<N, T>& clip_plane)
{
        return clip_plane.distance(geometry::spatial::offset_point(clip_plane.n, vertex)) >= 0;
}

template <std::size_t N, typename T, typename MeshType>
[[nodiscard]] bool facet_inside_clip_plane(
        const typename model::mesh::Mesh<N>::Facet& facet,
        const numerical::transform::MatrixVectorMultiplier<N + 1, T>& multiplier,
        const geometry::spatial::Hyperplane<N, T>& clip_plane,
        const std::vector<Vector<N, MeshType>>& vertices)
{
        for (const auto index : facet.vertices)
        {
                if (vertex_inside_clip_plane(multiplier(to_vector<T>(vertices[index])), clip_plane))
                {
                        return true;
                }
        }
        return false;
}

template <std::size_t N, typename T, typename MeshType>
[[nodiscard]] std::vector<typename model::mesh::Mesh<N>::Facet> find_facets_inside_clip_plane(
        const std::vector<Vector<N, MeshType>>& vertices,
        const std::vector<typename model::mesh::Mesh<N>::Facet>& facets,
        const Matrix<N + 1, N + 1, T>& mesh_matrix,
        const geometry::spatial::Hyperplane<N, T>& clip_plane)
{
        const numerical::transform::MatrixVectorMultiplier<N + 1, T> multiplier(mesh_matrix);

        std::vector<typename model::mesh::Mesh<N>::Facet> res;
        for (const typename model::mesh::Mesh<N>::Facet& facet : facets)
        {
                if (facet_inside_clip_plane(facet, multiplier, clip_plane, vertices))
                {
                        res.push_back(facet);
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<typename model::mesh::Mesh<N>::Facet> find_facets(
        const model::mesh::Reading<N>& mesh_object,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation)
{
        const model::mesh::Mesh<N>& mesh = mesh_object.mesh();

        if (!clip_plane_equation)
        {
                return mesh.facets;
        }

        return find_facets_inside_clip_plane(
                mesh.vertices, mesh.facets, to_matrix<T>(mesh_object.matrix()),
                geometry::spatial::Hyperplane<N, T>(*clip_plane_equation));
}
}

template <std::size_t N, typename T>
model::mesh::Mesh<N> optimize_mesh(
        const model::mesh::Reading<N>& mesh_object,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation)
{
        const model::mesh::Mesh<N>& mesh = mesh_object.mesh();

        model::mesh::Mesh<N> res;

        res.vertices = mesh.vertices;
        res.normals = mesh.normals;
        res.texcoords = mesh.texcoords;
        res.materials = mesh.materials;
        res.images = mesh.images;

        res.facets = find_facets(mesh_object, clip_plane_equation);

        return model::mesh::optimize(res);
}

#define TEMPLATE(N, T)                                 \
        template model::mesh::Mesh<(N)> optimize_mesh( \
                const model::mesh::Reading<(N)>&, const std::optional<Vector<(N) + 1, T>>&);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
