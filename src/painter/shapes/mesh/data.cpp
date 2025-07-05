/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "data.h"

#include "optimize.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::shapes::mesh
{
namespace
{
template <std::size_t N>
[[nodiscard]] std::array<int, N> add_offset(const std::array<int, N>& src, const int offset, const bool add)
{
        std::array<int, N> res;
        if (add)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = offset + src[i];
                }
        }
        else
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = -1;
                }
        }
        return res;
}

template <std::size_t N>
[[nodiscard]] std::array<int, N> add_offset(const std::array<int, N>& src, const int offset)
{
        std::array<int, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = offset + src[i];
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] std::array<numerical::Vector<N, T>, N> vertices_to_array(
        const std::vector<numerical::Vector<N, T>>& vertices,
        const std::array<int, N>& indices)
{
        std::array<numerical::Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = vertices[indices[i]];
        }
        return res;
}

template <std::size_t N, typename T, typename Color>
void write_vertices_and_normals(
        const model::mesh::Reading<N>& mesh_object,
        const model::mesh::Mesh<N>& mesh,
        MeshData<N, T, Color>* const data)
{
        const numerical::Matrix<N + 1, N + 1, T> mesh_matrix = numerical::to_matrix<T>(mesh_object.matrix());

        {
                const numerical::transform::MatrixVectorMultiplier multiplier(mesh_matrix);
                for (const auto& v : mesh.vertices)
                {
                        data->mesh.vertices.push_back(multiplier(to_vector<T>(v)));
                }
        }

        {
                const numerical::Matrix<N, N, T> matrix = mesh_matrix.template top_left<N, N>().inversed().transposed();
                for (const auto& v : mesh.normals)
                {
                        data->mesh.normals.push_back(matrix * to_vector<T>(v));
                }
        }
}

template <std::size_t N, typename T, typename Color>
void write_facets_and_materials(
        const model::mesh::Reading<N>& mesh_object,
        const model::mesh::Mesh<N>& mesh,
        const T alpha,
        const int vertices_offset,
        const int normals_offset,
        const int texcoords_offset,
        const int materials_offset,
        const int images_offset,
        MeshData<N, T, Color>* const data)
{
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

                data->mesh.facets.emplace_back(
                        vertices_to_array(data->mesh.vertices, vertices), data->mesh.normals, facet.has_normal, normals,
                        facet.has_texcoord, texcoords, material);

                data->facet_vertex_indices.push_back(vertices);

                facets_without_material = facets_without_material || no_material;
        }

        for (const typename model::mesh::Mesh<N>::Material& material : mesh.materials)
        {
                const int image = material.image < 0 ? -1 : (images_offset + material.image);
                data->mesh.materials.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), material.color, image, alpha);
        }

        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(data->mesh.materials.size()));
                data->mesh.materials.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1, alpha);
        }
}

template <std::size_t N, typename T, typename Color>
void add_mesh(
        const model::mesh::Reading<N>& mesh_object,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        MeshData<N, T, Color>* const data)
{
        const T alpha = std::clamp<T>(mesh_object.alpha(), 0, 1);

        if (alpha == 0)
        {
                return;
        }

        const model::mesh::Mesh<N> mesh = optimize_mesh(mesh_object, clip_plane_equation);

        if (mesh.vertices.empty())
        {
                return;
        }

        if (mesh.facets.empty())
        {
                return;
        }

        const int vertices_offset = data->mesh.vertices.size();
        const int normals_offset = data->mesh.normals.size();
        const int texcoords_offset = data->mesh.texcoords.size();
        const int materials_offset = data->mesh.materials.size();
        const int images_offset = data->mesh.images.size();

        write_vertices_and_normals(mesh_object, mesh, data);

        write_facets_and_materials(
                mesh_object, mesh, alpha, vertices_offset, normals_offset, texcoords_offset, materials_offset,
                images_offset, data);

        for (const auto& v : mesh.texcoords)
        {
                data->mesh.texcoords.push_back(to_vector<T>(v));
        }

        for (const auto& image : mesh.images)
        {
                data->mesh.images.emplace_back(image);
        }
}

template <std::size_t N, typename T, typename Color>
MeshData<N, T, Color> create_mesh_data(
        const std::vector<model::mesh::Reading<N>>& mesh_objects,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        MeshData<N, T, Color> data;

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
                image_count += mesh.mesh().images.size();
                facet_count += mesh.mesh().facets.size();
                material_count += mesh.mesh().materials.size();

                for (const typename model::mesh::Mesh<N>::Facet& facet : mesh.mesh().facets)
                {
                        if (facet.material < 0)
                        {
                                ++material_count;
                                break;
                        }
                }
        }

        data.mesh.vertices.reserve(vertex_count);
        data.mesh.normals.reserve(normal_count);
        data.mesh.texcoords.reserve(texcoord_count);
        data.mesh.materials.reserve(material_count);
        data.mesh.images.reserve(image_count);
        data.mesh.facets.reserve(facet_count);
        data.facet_vertex_indices.reserve(facet_count);

        for (const model::mesh::Reading<N>& mesh_object : mesh_objects)
        {
                add_mesh(mesh_object, clip_plane_equation, &data);
        }

        if (data.mesh.facets.empty())
        {
                error("No facets found in meshes");
        }

        return data;
}
}

template <std::size_t N, typename T, typename Color>
MeshData<N, T, Color> create_mesh_data(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        const bool write_log)
{
        const std::optional<Clock::time_point> start_time = [&] -> std::optional<Clock::time_point>
        {
                if (write_log)
                {
                        return Clock::now();
                }
                return std::nullopt;
        }();

        std::vector<model::mesh::Reading<N>> reading;
        reading.reserve(mesh_objects.size());
        for (const model::mesh::MeshObject<N>* const mesh_object : mesh_objects)
        {
                reading.emplace_back(*mesh_object);
        }

        MeshData<N, T, Color> res = create_mesh_data<N, T, Color>(reading, clip_plane_equation);

        ASSERT(write_log == start_time.has_value());
        if (write_log)
        {
                LOG("Painter mesh data created, " + to_string_fixed(duration_from(*start_time), 5)
                    + " s, vertex count = " + to_string_digit_groups(res.mesh.vertices.size())
                    + ", facet count = " + to_string_digit_groups(res.mesh.facets.size()));
        }

        return res;
}

#define TEMPLATE(N, T, C)                                                \
        template MeshData<N, T, C> create_mesh_data(                     \
                const std::vector<const model::mesh::MeshObject<(N)>*>&, \
                const std::optional<numerical::Vector<(N) + 1, T>>&, const bool);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
