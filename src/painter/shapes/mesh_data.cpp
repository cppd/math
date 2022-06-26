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
}

template <std::size_t N, typename T, typename Color>
void MeshData<N, T, Color>::create(const model::mesh::Reading<N>& mesh_object)
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

        const int vertices_offset = vertices_.size();
        const int normals_offset = normals_.size();
        const int texcoords_offset = texcoords_.size();
        const int materials_offset = materials_.size();
        const int images_offset = images_.size();

        {
                const std::vector<Vector<N, T>>& vertices = to_vector<T>(mesh.vertices);
                vertices_.insert(vertices_.cend(), vertices.cbegin(), vertices.cend());
        }
        {
                const auto iter_begin = std::next(vertices_.begin(), vertices_offset);
                const auto iter_end = vertices_.end();
                std::transform(
                        iter_begin, iter_end, iter_begin,
                        numerical::transform::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));
        }
        {
                const std::vector<Vector<N, T>>& normals = to_vector<T>(mesh.normals);
                normals_.insert(normals_.cend(), normals.cbegin(), normals.cend());
        }
        {
                const std::vector<Vector<N - 1, T>>& texcoords = to_vector<T>(mesh.texcoords);
                texcoords_.insert(texcoords_.cend(), texcoords.cbegin(), texcoords.cend());
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

                facets_.emplace_back(
                        vertices_to_array(vertices_, vertices), normals_, facet.has_normal, normals, facet.has_texcoord,
                        texcoords, material);
                facet_vertex_indices_.push_back(vertices);

                facets_without_material = facets_without_material || no_material;
        }

        for (const typename model::mesh::Mesh<N>::Material& m : mesh.materials)
        {
                const int image = m.image < 0 ? -1 : (images_offset + m.image);
                materials_.emplace_back(mesh_object.metalness(), mesh_object.roughness(), m.color, image, alpha);
        }

        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(materials_.size()));
                materials_.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1, alpha);
        }

        for (const image::Image<N - 1>& image : mesh.images)
        {
                images_.emplace_back(image);
        }
}

template <std::size_t N, typename T, typename Color>
void MeshData<N, T, Color>::create(
        const std::vector<model::mesh::Reading<N>>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& /*clip_plane_equation*/)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        vertices_.clear();
        normals_.clear();
        texcoords_.clear();
        materials_.clear();
        images_.clear();
        facets_.clear();
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
        vertices_.reserve(vertex_count);
        normals_.reserve(normal_count);
        texcoords_.reserve(texcoord_count);
        materials_.reserve(material_count);
        images_.reserve(image_count);
        facets_.reserve(facet_count);

        for (const model::mesh::Reading<N>& mesh_object : mesh_objects)
        {
                create(mesh_object);
        }

        if (facets_.empty())
        {
                error("No facets found in meshes");
        }
}

template <std::size_t N, typename T, typename Color>
MeshData<N, T, Color>::MeshData(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        const bool write_log)
{
        const Clock::time_point start_time = Clock::now();

        std::vector<model::mesh::Reading<N>> reading;
        reading.reserve(mesh_objects.size());
        for (const model::mesh::MeshObject<N>* const mesh_object : mesh_objects)
        {
                reading.emplace_back(*mesh_object);
        }
        create(reading, clip_plane_equation);

        const double duration = duration_from(start_time);

        if (write_log)
        {
                LOG("Painter mesh data created, " + to_string_fixed(duration, 5)
                    + " s, vertex count = " + to_string_digit_groups(vertices_.size())
                    + ", facet count = " + to_string_digit_groups(facets_.size()));
        }
}

#define TEMPLATE(N, T, C) template class MeshData<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
