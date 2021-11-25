/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "mesh_facet.h"
#include "mesh_texture.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/model/mesh_object.h>
#include <src/numerical/transform.h>

#include <algorithm>
#include <vector>

namespace ns::painter
{
namespace mesh_data_implementation
{
template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, const int offset, const bool add)
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
std::array<int, N> add_offset(const std::array<int, N>& src, const int offset)
{
        std::array<int, N> r;
        for (unsigned i = 0; i < N; ++i)
        {
                r[i] = offset + src[i];
        }
        return r;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> vertices_to_array(
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

template <typename T, typename Color>
struct Material
{
        T metalness;
        T roughness;
        Color color;
        T alpha;
        int image;

        Material(const T metalness, const T roughness, const color::Color& color, const int image, const T alpha)
                : metalness(std::clamp(metalness, T(0), T(1))),
                  roughness(std::clamp(roughness, T(0), T(1))),
                  color(color.to_color<Color>().clamp(0, 1)),
                  alpha(std::clamp(alpha, T(0), T(1))),
                  image(image)
        {
        }
};

template <std::size_t N, typename T, typename Color>
class MeshData final
{
        std::vector<Vector<N, T>> vertices_;
        std::vector<Vector<N, T>> normals_;
        std::vector<Vector<N - 1, T>> texcoords_;
        std::vector<Material<T, Color>> materials_;
        std::vector<MeshTexture<N - 1>> images_;
        std::vector<MeshFacet<N, T>> facets_;
        std::vector<std::array<int, N>> facet_vertex_indices_;

        void create(const mesh::Reading<N>& mesh_object)
        {
                namespace impl = mesh_data_implementation;

                const T alpha = std::clamp<T>(mesh_object.alpha(), 0, 1);

                if (alpha == 0)
                {
                        return;
                }

                const mesh::Mesh<N>& mesh = mesh_object.mesh();

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
                        auto iter_begin = std::next(vertices_.begin(), vertices_offset);
                        auto iter_end = vertices_.end();
                        std::transform(
                                iter_begin, iter_end, iter_begin,
                                matrix::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));
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

                for (const typename mesh::Mesh<N>::Facet& facet : mesh.facets)
                {
                        const bool no_material = facet.material < 0;
                        const int facet_material = no_material ? default_material_index : facet.material;

                        const std::array<int, N> vertices = impl::add_offset(facet.vertices, vertices_offset);
                        const std::array<int, N> normals =
                                impl::add_offset(facet.normals, normals_offset, facet.has_normal);
                        const std::array<int, N> texcoords =
                                impl::add_offset(facet.texcoords, texcoords_offset, facet.has_texcoord);
                        const int material = facet_material + materials_offset;

                        facets_.emplace_back(
                                impl::vertices_to_array(vertices_, vertices), normals_, facet.has_normal, normals,
                                facet.has_texcoord, texcoords, material);
                        facet_vertex_indices_.push_back(vertices);

                        facets_without_material = facets_without_material || no_material;
                }

                for (const typename mesh::Mesh<N>::Material& m : mesh.materials)
                {
                        const int image = m.image < 0 ? -1 : (images_offset + m.image);
                        materials_.emplace_back(
                                mesh_object.metalness(), mesh_object.roughness(), m.color, image, alpha);
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

        void create(const std::vector<mesh::Reading<N>>& mesh_objects)
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
                for (const mesh::Reading<N>& mesh : mesh_objects)
                {
                        vertex_count += mesh.mesh().vertices.size();
                        normal_count += mesh.mesh().normals.size();
                        texcoord_count += mesh.mesh().texcoords.size();
                        bool no_material = false;
                        for (const typename mesh::Mesh<N>::Facet& facet : mesh.mesh().facets)
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

                for (const mesh::Reading<N>& mesh_object : mesh_objects)
                {
                        create(mesh_object);
                }

                ASSERT(vertex_count == vertices_.size());
                ASSERT(normal_count == normals_.size());
                ASSERT(texcoord_count == texcoords_.size());
                ASSERT(material_count == materials_.size());
                ASSERT(image_count == images_.size());
                ASSERT(facet_count == facets_.size());

                if (facets_.empty())
                {
                        error("No facets found in meshes");
                }
        }

public:
        MeshData(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, const bool write_log)
        {
                const Clock::time_point start_time = Clock::now();

                std::vector<mesh::Reading<N>> reading;
                reading.reserve(mesh_objects.size());
                for (const mesh::MeshObject<N>* mesh_object : mesh_objects)
                {
                        reading.emplace_back(*mesh_object);
                }
                create(reading);

                const double duration = duration_from(start_time);

                if (write_log)
                {
                        LOG("Painter mesh data created, " + to_string_fixed(duration, 5)
                            + " s, vertex count = " + to_string_digit_groups(vertices_.size())
                            + ", facet count = " + to_string_digit_groups(facets_.size()));
                }
        }

        const std::vector<Vector<N, T>>& normals() const
        {
                return normals_;
        }

        const std::vector<Vector<N - 1, T>>& texcoords() const
        {
                return texcoords_;
        }

        const std::vector<Material<T, Color>>& materials() const
        {
                return materials_;
        }

        const std::vector<MeshTexture<N - 1>>& images() const
        {
                return images_;
        }

        const std::vector<MeshFacet<N, T>>& facets() const
        {
                return facets_;
        }

        geometry::BoundingBox<N, T> facet_bounding_box(const std::size_t facet_index) const
        {
                ASSERT(facet_index < facet_vertex_indices_.size());
                return geometry::BoundingBox(vertices_, facet_vertex_indices_[facet_index]);
        }
};
}