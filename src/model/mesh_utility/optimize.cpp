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

#include "optimize.h"

#include <src/settings/instantiation.h>

#include <unordered_map>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N>
using MeshFacet = typename Mesh<N>::Facet;

template <std::size_t N>
using MeshPoint = typename Mesh<N>::Point;

template <std::size_t N>
using MeshLine = typename Mesh<N>::Line;

template <std::size_t N>
using MeshMaterial = typename Mesh<N>::Material;

//

template <std::size_t N>
void insert_vertices(const MeshFacet<N>& facet, std::unordered_map<int, int>* const vertices, int* const idx_vertices)
{
        for (const int index : facet.vertices)
        {
                if (vertices->try_emplace(index, *idx_vertices).second)
                {
                        ++(*idx_vertices);
                }
        }
}

template <std::size_t N>
void insert_normals(const MeshFacet<N>& facet, std::unordered_map<int, int>* const normals, int* const idx_normals)
{
        if (!facet.has_normal)
        {
                return;
        }

        for (const int index : facet.normals)
        {
                ASSERT(index >= 0);
                if (normals->try_emplace(index, *idx_normals).second)
                {
                        ++(*idx_normals);
                }
        }
}

template <std::size_t N>
void insert_texcoords(
        const MeshFacet<N>& facet,
        std::unordered_map<int, int>* const texcoords,
        int* const idx_texcoords)
{
        if (!facet.has_texcoord)
        {
                return;
        }

        for (const int index : facet.texcoords)
        {
                ASSERT(index >= 0);
                if (texcoords->try_emplace(index, *idx_texcoords).second)
                {
                        ++(*idx_texcoords);
                }
        }
}

template <std::size_t N>
void insert_materials(
        const MeshFacet<N>& facet,
        const std::vector<MeshMaterial<N>>& mesh_materials,
        std::unordered_map<int, int>* const materials,
        int* const idx_materials,
        std::unordered_map<int, int>* const images,
        int* const idx_images)
{
        if (facet.material < 0)
        {
                return;
        }

        if (materials->try_emplace(facet.material, *idx_materials).second)
        {
                ++(*idx_materials);

                const int image = mesh_materials[facet.material].image;
                if (image < 0)
                {
                        return;
                }

                if (images->try_emplace(image, *idx_images).second)
                {
                        ++(*idx_images);
                }
        }
}

template <std::size_t N>
void insert_vertices(const MeshPoint<N>& point, std::unordered_map<int, int>* const vertices, int* const idx_vertices)
{
        if (vertices->try_emplace(point.vertex, *idx_vertices).second)
        {
                ++(*idx_vertices);
        }
}

template <std::size_t N>
void insert_vertices(const MeshLine<N>& line, std::unordered_map<int, int>* const vertices, int* const idx_vertices)
{
        for (const int index : line.vertices)
        {
                if (vertices->try_emplace(index, *idx_vertices).second)
                {
                        ++(*idx_vertices);
                }
        }
}

//

template <std::size_t N>
void fill_mesh(
        const std::unordered_map<int, int>& vertices,
        const std::unordered_map<int, int>& normals,
        const std::unordered_map<int, int>& texcoords,
        const std::unordered_map<int, int>& materials,
        const std::unordered_map<int, int>& images,
        const Mesh<N>& src_mesh,
        Mesh<N>* const mesh)
{
        mesh->vertices.resize(vertices.size());
        for (const auto& [old_index, new_index] : vertices)
        {
                mesh->vertices[new_index] = src_mesh.vertices[old_index];
        }

        mesh->normals.resize(normals.size());
        for (const auto& [old_index, new_index] : normals)
        {
                mesh->normals[new_index] = src_mesh.normals[old_index];
        }

        mesh->texcoords.resize(texcoords.size());
        for (const auto& [old_index, new_index] : texcoords)
        {
                mesh->texcoords[new_index] = src_mesh.texcoords[old_index];
        }

        mesh->materials.resize(materials.size());
        for (const auto& [old_index, new_index] : materials)
        {
                mesh->materials[new_index] = src_mesh.materials[old_index];
                const int old_image_index = src_mesh.materials[old_index].image;
                if (old_image_index >= 0)
                {
                        const auto iter = images.find(old_image_index);
                        ASSERT(iter != images.cend());
                        mesh->materials[new_index].image = iter->second;
                }
        }

        mesh->images.resize(images.size());
        for (const auto& [old_index, new_index] : images)
        {
                mesh->images[new_index] = src_mesh.images[old_index];
        }
}

template <std::size_t N>
void fill_facet_vertices(
        const std::unordered_map<int, int>& vertices,
        const MeshFacet<N>& src_facet,
        MeshFacet<N>* const facet)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto iter = vertices.find(src_facet.vertices[i]);
                ASSERT(iter != vertices.cend());
                facet->vertices[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_normals(
        const std::unordered_map<int, int>& normals,
        const MeshFacet<N>& src_facet,
        MeshFacet<N>* const facet)
{
        if (!src_facet.has_normal)
        {
                facet->has_normal = false;
                for (std::size_t i = 0; i < N; ++i)
                {
                        facet->normals[i] = -1;
                }
                return;
        }

        facet->has_normal = true;
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto iter = normals.find(src_facet.normals[i]);
                ASSERT(iter != normals.cend());
                facet->normals[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_texcoords(
        const std::unordered_map<int, int>& texcoords,
        const MeshFacet<N>& src_facet,
        MeshFacet<N>* const facet)
{
        if (!src_facet.has_texcoord)
        {
                facet->has_texcoord = false;
                for (std::size_t i = 0; i < N; ++i)
                {
                        facet->texcoords[i] = -1;
                }
                return;
        }

        facet->has_texcoord = true;
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto iter = texcoords.find(src_facet.texcoords[i]);
                ASSERT(iter != texcoords.cend());
                facet->texcoords[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_materials(
        const std::unordered_map<int, int>& materials,
        const MeshFacet<N>& src_facet,
        MeshFacet<N>* const facet)
{
        if (src_facet.material < 0)
        {
                facet->material = -1;
                return;
        }

        const auto iter = materials.find(src_facet.material);
        ASSERT(iter != materials.cend());
        facet->material = iter->second;
}

template <std::size_t N>
MeshFacet<N> make_facet(
        const std::unordered_map<int, int>& vertices,
        const std::unordered_map<int, int>& normals,
        const std::unordered_map<int, int>& texcoords,
        const std::unordered_map<int, int>& materials,
        const MeshFacet<N>& facet)
{
        MeshFacet<N> res;

        fill_facet_vertices<N>(vertices, facet, &res);
        fill_facet_normals<N>(normals, facet, &res);
        fill_facet_texcoords<N>(texcoords, facet, &res);
        fill_facet_materials<N>(materials, facet, &res);

        return res;
}

template <std::size_t N>
MeshPoint<N> make_point(const std::unordered_map<int, int>& vertices, const MeshPoint<N>& point)
{
        MeshPoint<N> res;

        const auto iter = vertices.find(point.vertex);
        ASSERT(iter != vertices.cend());
        res.vertex = iter->second;

        return res;
}

template <std::size_t N>
MeshLine<N> make_line(const std::unordered_map<int, int>& vertices, const MeshLine<N>& line)
{
        MeshLine<N> res;

        for (std::size_t i = 0; i < 2; ++i)
        {
                const auto iter = vertices.find(line.vertices[i]);
                ASSERT(iter != vertices.cend());
                res.vertices[i] = iter->second;
        }

        return res;
}
}

template <std::size_t N>
Mesh<N> optimize(const Mesh<N>& mesh)
{
        std::unordered_map<int, int> vertices;
        std::unordered_map<int, int> normals;
        std::unordered_map<int, int> texcoords;
        std::unordered_map<int, int> materials;
        std::unordered_map<int, int> images;

        int idx_vertices = 0;
        int idx_normals = 0;
        int idx_texcoords = 0;
        int idx_materials = 0;
        int idx_images = 0;

        for (const MeshFacet<N>& facet : mesh.facets)
        {
                insert_vertices<N>(facet, &vertices, &idx_vertices);
                insert_normals<N>(facet, &normals, &idx_normals);
                insert_texcoords<N>(facet, &texcoords, &idx_texcoords);
                insert_materials<N>(facet, mesh.materials, &materials, &idx_materials, &images, &idx_images);
        }

        for (const MeshPoint<N>& point : mesh.points)
        {
                insert_vertices<N>(point, &vertices, &idx_vertices);
        }

        for (const MeshLine<N>& line : mesh.lines)
        {
                insert_vertices<N>(line, &vertices, &idx_vertices);
        }

        ASSERT(idx_vertices == static_cast<int>(vertices.size()));
        ASSERT(idx_normals == static_cast<int>(normals.size()));
        ASSERT(idx_texcoords == static_cast<int>(texcoords.size()));
        ASSERT(idx_materials == static_cast<int>(materials.size()));
        ASSERT(idx_images == static_cast<int>(images.size()));

        Mesh<N> res;

        fill_mesh(vertices, normals, texcoords, materials, images, mesh, &res);

        for (const MeshFacet<N>& facet : mesh.facets)
        {
                res.facets.push_back(make_facet<N>(vertices, normals, texcoords, materials, facet));
        }

        for (const MeshPoint<N>& point : mesh.points)
        {
                res.points.push_back(make_point<N>(vertices, point));
        }

        for (const MeshLine<N>& line : mesh.lines)
        {
                res.lines.push_back(make_line<N>(vertices, line));
        }

        return res;
}

#define TEMPLATE(N) template Mesh<(N)> optimize(const Mesh<(N)>&);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
