/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "mesh_data.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/transform.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <optional>

namespace ns::painter
{
namespace
{
template <std::size_t N>
[[nodiscard]] std::array<int, N> add_offset(const std::array<int, N>& src, const int offset, const bool add)
{
        std::array<int, N> r;
        if (add)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = offset + src[i];
                }
        }
        else
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = -1;
                }
        }
        return r;
}

template <std::size_t N>
[[nodiscard]] std::array<int, N> add_offset(const std::array<int, N>& src, const int offset)
{
        std::array<int, N> r;
        for (unsigned i = 0; i < N; ++i)
        {
                r[i] = offset + src[i];
        }
        return r;
}

template <std::size_t N, typename T>
[[nodiscard]] std::array<Vector<N, T>, N> vertices_to_array(
        const std::vector<Vector<N, T>>& vertices,
        const std::array<int, N>& indices)
{
        std::array<Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = vertices[indices[i]];
        }
        return res;
}

template <std::size_t N, typename T, typename Color>
void create(
        const model::mesh::Reading<N>& mesh_object,
        MeshData<N, T, Color>* const mesh_data,
        std::vector<std::array<int, N>>* const facet_vertex_indices)
{
        const T alpha = std::clamp<T>(mesh_object.alpha(), 0, 1);

        if (alpha == 0)
        {
                return;
        }

        const model::mesh::Mesh<N> mesh = model::mesh::optimize(mesh_object.mesh());

        if (mesh.vertices.empty())
        {
                return;
        }

        if (mesh.facets.empty())
        {
                return;
        }

        const int vertices_offset = mesh_data->vertices.size();
        const int normals_offset = mesh_data->normals.size();
        const int texcoords_offset = mesh_data->texcoords.size();
        const int materials_offset = mesh_data->materials.size();
        const int images_offset = mesh_data->images.size();

        const Matrix<N + 1, N + 1, T> mesh_matrix = to_matrix<T>(mesh_object.matrix());

        {
                const numerical::transform::MatrixVectorMultiplier<N + 1, T> multiplier(mesh_matrix);
                for (const auto& v : mesh.vertices)
                {
                        mesh_data->vertices.push_back(multiplier(to_vector<T>(v)));
                }
        }

        {
                const Matrix<N, N, T> matrix = mesh_matrix.template top_left<N, N>().inverse().transpose();
                for (const auto& v : mesh.normals)
                {
                        mesh_data->normals.push_back(matrix * to_vector<T>(v));
                }
        }

        for (const auto& v : mesh.texcoords)
        {
                mesh_data->texcoords.push_back(to_vector<T>(v));
        }

        const int default_material_index = mesh.materials.size();

        bool facets_without_material = false;

        for (const typename model::mesh::Mesh<N>::Facet& facet : mesh.facets)
        {
                const bool no_material = facet.material < 0;
                const int facet_material = no_material ? default_material_index : facet.material;

                const std::array<int, N> vertices = add_offset(facet.vertices, vertices_offset);
                const std::array<int, N> normals = add_offset(facet.normals, normals_offset, facet.has_normal);
                const std::array<int, N> texcoords = add_offset(facet.texcoords, texcoords_offset, facet.has_texcoord);
                const int material = facet_material + materials_offset;

                mesh_data->facets.emplace_back(
                        vertices_to_array(mesh_data->vertices, vertices), mesh_data->normals, facet.has_normal, normals,
                        facet.has_texcoord, texcoords, material);

                facet_vertex_indices->push_back(vertices);

                facets_without_material = facets_without_material || no_material;
        }

        for (const typename model::mesh::Mesh<N>::Material& m : mesh.materials)
        {
                const int image = m.image < 0 ? -1 : (images_offset + m.image);
                mesh_data->materials.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), m.color, image, alpha);
        }

        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(mesh_data->materials.size()));
                mesh_data->materials.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1, alpha);
        }

        for (const image::Image<N - 1>& image : mesh.images)
        {
                mesh_data->images.emplace_back(image);
        }
}

template <std::size_t N, typename T, typename Color>
void create(
        const std::vector<model::mesh::Reading<N>>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& /*clip_plane_equation*/,
        MeshData<N, T, Color>* const mesh_data,
        std::vector<std::array<int, N>>* const facet_vertex_indices)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        mesh_data->vertices.clear();
        mesh_data->normals.clear();
        mesh_data->texcoords.clear();
        mesh_data->materials.clear();
        mesh_data->images.clear();
        mesh_data->facets.clear();
        facet_vertex_indices->clear();
        std::size_t vertex_count = 0;
        std::size_t normal_count = 0;
        std::size_t texcoord_count = 0;
        std::size_t material_count = 0;
        std::size_t image_count = 0;
        std::size_t facet_count = 0;
        for (const model::mesh::Reading<N>& mesh : mesh_objects)
        {
                vertex_count += mesh.mesh().vertices.size();
                normal_count += mesh.mesh().normals.size();
                texcoord_count += mesh.mesh().texcoords.size();
                bool no_material = false;
                for (const typename model::mesh::Mesh<N>::Facet& facet : mesh.mesh().facets)
                {
                        if (facet.material < 0)
                        {
                                no_material = true;
                                break;
                        }
                }
                material_count += mesh.mesh().materials.size() + (no_material ? 1 : 0);
                image_count += mesh.mesh().images.size();
                facet_count += mesh.mesh().facets.size();
        }
        mesh_data->vertices.reserve(vertex_count);
        mesh_data->normals.reserve(normal_count);
        mesh_data->texcoords.reserve(texcoord_count);
        mesh_data->materials.reserve(material_count);
        mesh_data->images.reserve(image_count);
        mesh_data->facets.reserve(facet_count);
        facet_vertex_indices->reserve(facet_count);

        for (const model::mesh::Reading<N>& mesh_object : mesh_objects)
        {
                create(mesh_object, mesh_data, facet_vertex_indices);
        }

        if (mesh_data->facets.empty())
        {
                error("No facets found in meshes");
        }
}
}

template <std::size_t N, typename T, typename Color>
MeshInfo<N, T, Color> create_mesh_info(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        const bool write_log)
{
        const std::optional<Clock::time_point> start_time = [&]() -> std::optional<Clock::time_point>
        {
                if (write_log)
                {
                        return Clock::now();
                }
                return std::nullopt;
        }();

        MeshInfo<N, T, Color> res;

        std::vector<model::mesh::Reading<N>> reading;
        reading.reserve(mesh_objects.size());
        for (const model::mesh::MeshObject<N>* const mesh_object : mesh_objects)
        {
                reading.emplace_back(*mesh_object);
        }

        create(reading, clip_plane_equation, &res.mesh_data, &res.facet_vertex_indices);

        if (write_log)
        {
                LOG("Painter mesh data created, " + to_string_fixed(duration_from(*start_time), 5)
                    + " s, vertex count = " + to_string_digit_groups(res.mesh_data.vertices.size())
                    + ", facet count = " + to_string_digit_groups(res.mesh_data.facets.size()));
        }

        return res;
}

#define TEMPLATE(N, T, C)                                                                                          \
        template MeshInfo<N, T, C> create_mesh_info(                                                               \
                const std::vector<const model::mesh::MeshObject<(N)>*>&, const std::optional<Vector<(N) + 1, T>>&, \
                const bool);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
