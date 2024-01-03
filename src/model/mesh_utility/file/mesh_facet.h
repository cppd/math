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

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::model::mesh::file
{
namespace mesh_check_implementation
{
template <std::size_t N>
void check_facet_indices(
        const unsigned index,
        const int vertex_count,
        const int texcoord_count,
        const int normal_count,
        const typename Mesh<N>::Facet& facet)
{
        if (facet.vertices[index] < 0 || facet.vertices[index] >= vertex_count)
        {
                error("Vertex index " + to_string(facet.vertices[index]) + " is out of bounds [0, "
                      + to_string(vertex_count) + ")");
        }

        if (facet.has_texcoord)
        {
                if (facet.texcoords[index] < 0 || facet.texcoords[index] >= texcoord_count)
                {
                        error("Texture coordinate index " + to_string(facet.texcoords[index]) + " is out of bounds [0, "
                              + to_string(texcoord_count) + ")");
                }
        }
        else if (facet.texcoords[index] != -1)
        {
                error("No texture but texture coordinate index is not set to -1");
        }

        if (facet.has_normal)
        {
                if (facet.normals[index] < 0 || facet.normals[index] >= normal_count)
                {
                        error("Normal index " + to_string(facet.normals[index]) + " is out of bounds [0, "
                              + to_string(normal_count) + ")");
                }
        }
        else if (facet.normals[index] != -1)
        {
                error("No normals but normal coordinate index is not set to -1");
        }
}

template <std::size_t N>
void check_facet_indices(const Mesh<N>& mesh)
{
        const int vertex_count = mesh.vertices.size();
        const int texcoord_count = mesh.texcoords.size();
        const int normal_count = mesh.normals.size();

        for (const typename Mesh<N>::Facet& facet : mesh.facets)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        check_facet_indices<N>(i, vertex_count, texcoord_count, normal_count, facet);
                }
        }
}

template <std::size_t N>
        requires (N == 3)
[[nodiscard]] bool facet_dimension_is_correct(
        const std::vector<Vector<N, float>>& vertices,
        const std::array<int, N>& indices)
{
        const Vector<3, double> e0 = to_vector<double>(vertices[indices[1]] - vertices[indices[0]]);
        const Vector<3, double> e1 = to_vector<double>(vertices[indices[2]] - vertices[indices[0]]);

        if (e0[1] * e1[2] - e0[2] * e1[1] != 0)
        {
                return true;
        }

        if (e0[0] * e1[2] - e0[2] * e1[0] != 0)
        {
                return true;
        }

        if (e0[0] * e1[1] - e0[1] * e1[0] != 0)
        {
                return true;
        }

        return false;
}

template <std::size_t N>
void remove_facets_with_incorrect_dimension(Mesh<N>* const mesh)
{
        static constexpr bool CHECK =
                requires { facet_dimension_is_correct(mesh->vertices, mesh->facets.front().vertices); };

        if constexpr (!CHECK)
        {
                return;
        }
        else
        {
                std::vector<bool> wrong_facets(mesh->facets.size(), false);

                int wrong_facet_count = 0;

                for (std::size_t i = 0; i < mesh->facets.size(); ++i)
                {
                        if (!facet_dimension_is_correct(mesh->vertices, mesh->facets[i].vertices))
                        {
                                wrong_facets[i] = true;
                                ++wrong_facet_count;
                        }
                }

                if (wrong_facet_count == 0)
                {
                        return;
                }

                std::vector<typename Mesh<N>::Facet> facets;
                facets.reserve(mesh->facets.size() - wrong_facet_count);

                for (std::size_t i = 0; i < mesh->facets.size(); ++i)
                {
                        if (!wrong_facets[i])
                        {
                                facets.push_back(mesh->facets[i]);
                        }
                }

                mesh->facets = std::move(facets);
        }
}

template <std::size_t N>
void remove_incorrect_facets(Mesh<N>* const mesh)
{
        const std::size_t facet_count = mesh->facets.size();

        remove_facets_with_incorrect_dimension(mesh);

        if (facet_count == mesh->facets.size())
        {
                return;
        }

        if (mesh->facets.empty())
        {
                error("No " + to_string(N - 1) + "-facet found in " + to_string(N) + "-mesh");
        }
}
}

template <std::size_t N>
void check_and_correct_mesh_facets(Mesh<N>* const mesh)
{
        namespace impl = mesh_check_implementation;

        impl::check_facet_indices(*mesh);
        impl::remove_incorrect_facets(mesh);
}
}
