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
void insert_materials_and_images(
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

//

struct Maps final
{
        std::unordered_map<int, int> vertices;
        std::unordered_map<int, int> normals;
        std::unordered_map<int, int> texcoords;
        std::unordered_map<int, int> materials;
        std::unordered_map<int, int> images;
};

template <std::size_t N>
Maps create_maps(const Mesh<N>& mesh)
{
        Maps res;

        int idx_vertices = 0;
        int idx_normals = 0;
        int idx_texcoords = 0;
        int idx_materials = 0;
        int idx_images = 0;

        for (const MeshFacet<N>& facet : mesh.facets)
        {
                insert_vertices<N>(facet, &res.vertices, &idx_vertices);
                insert_normals<N>(facet, &res.normals, &idx_normals);
                insert_texcoords<N>(facet, &res.texcoords, &idx_texcoords);
                insert_materials_and_images<N>(
                        facet, mesh.materials, &res.materials, &idx_materials, &res.images, &idx_images);
        }

        for (const MeshPoint<N>& point : mesh.points)
        {
                insert_vertices<N>(point, &res.vertices, &idx_vertices);
        }

        for (const MeshLine<N>& line : mesh.lines)
        {
                insert_vertices<N>(line, &res.vertices, &idx_vertices);
        }

        ASSERT(idx_vertices == static_cast<int>(res.vertices.size()));
        ASSERT(idx_normals == static_cast<int>(res.normals.size()));
        ASSERT(idx_texcoords == static_cast<int>(res.texcoords.size()));
        ASSERT(idx_materials == static_cast<int>(res.materials.size()));
        ASSERT(idx_images == static_cast<int>(res.images.size()));

        return res;
}

//

template <std::size_t N>
void fill_facet_vertices(const Maps& maps, const MeshFacet<N>& src_facet, MeshFacet<N>* const facet)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto iter = maps.vertices.find(src_facet.vertices[i]);
                ASSERT(iter != maps.vertices.cend());
                facet->vertices[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_normals(const Maps& maps, const MeshFacet<N>& src_facet, MeshFacet<N>* const facet)
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
                const auto iter = maps.normals.find(src_facet.normals[i]);
                ASSERT(iter != maps.normals.cend());
                facet->normals[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_texcoords(const Maps& maps, const MeshFacet<N>& src_facet, MeshFacet<N>* const facet)
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
                const auto iter = maps.texcoords.find(src_facet.texcoords[i]);
                ASSERT(iter != maps.texcoords.cend());
                facet->texcoords[i] = iter->second;
        }
}

template <std::size_t N>
void fill_facet_materials(const Maps& maps, const MeshFacet<N>& src_facet, MeshFacet<N>* const facet)
{
        if (src_facet.material < 0)
        {
                facet->material = -1;
                return;
        }

        const auto iter = maps.materials.find(src_facet.material);
        ASSERT(iter != maps.materials.cend());
        facet->material = iter->second;
}

//

template <std::size_t N>
void fill_mesh_vertices(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->vertices.resize(maps.vertices.size());
        for (const auto& [old_index, new_index] : maps.vertices)
        {
                mesh->vertices[new_index] = src_mesh.vertices[old_index];
        }
}

template <std::size_t N>
void fill_mesh_normals(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->normals.resize(maps.normals.size());
        for (const auto& [old_index, new_index] : maps.normals)
        {
                mesh->normals[new_index] = src_mesh.normals[old_index];
        }
}

template <std::size_t N>
void fill_mesh_texcoords(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->texcoords.resize(maps.texcoords.size());
        for (const auto& [old_index, new_index] : maps.texcoords)
        {
                mesh->texcoords[new_index] = src_mesh.texcoords[old_index];
        }
}

template <std::size_t N>
void fill_mesh_materials(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->materials.resize(maps.materials.size());
        for (const auto& [old_index, new_index] : maps.materials)
        {
                mesh->materials[new_index] = src_mesh.materials[old_index];
                const int old_image_index = src_mesh.materials[old_index].image;
                if (old_image_index >= 0)
                {
                        const auto iter = maps.images.find(old_image_index);
                        ASSERT(iter != maps.images.cend());
                        mesh->materials[new_index].image = iter->second;
                }
        }
}

template <std::size_t N>
void fill_mesh_images(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->images.resize(maps.images.size());
        for (const auto& [old_index, new_index] : maps.images)
        {
                mesh->images[new_index] = src_mesh.images[old_index];
        }
}

template <std::size_t N>
void fill_mesh_facets(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->facets.reserve(src_mesh.facets.size());
        for (const MeshFacet<N>& facet : src_mesh.facets)
        {
                MeshFacet<N>& f = mesh->facets.emplace_back();
                fill_facet_vertices<N>(maps, facet, &f);
                fill_facet_normals<N>(maps, facet, &f);
                fill_facet_texcoords<N>(maps, facet, &f);
                fill_facet_materials<N>(maps, facet, &f);
        }
}

template <std::size_t N>
void fill_mesh_points(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->points.reserve(src_mesh.points.size());
        for (const MeshPoint<N>& point : src_mesh.points)
        {
                MeshPoint<N>& p = mesh->points.emplace_back();
                const auto iter = maps.vertices.find(point.vertex);
                ASSERT(iter != maps.vertices.cend());
                p.vertex = iter->second;
        }
}

template <std::size_t N>
void fill_mesh_lines(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->lines.reserve(src_mesh.lines.size());
        for (const MeshLine<N>& line : src_mesh.lines)
        {
                MeshLine<N>& l = mesh->lines.emplace_back();
                for (std::size_t i = 0; i < 2; ++i)
                {
                        const auto iter = maps.vertices.find(line.vertices[i]);
                        ASSERT(iter != maps.vertices.cend());
                        l.vertices[i] = iter->second;
                }
        }
}
}

template <std::size_t N>
Mesh<N> optimize(const Mesh<N>& mesh)
{
        const Maps maps = create_maps(mesh);

        Mesh<N> res;

        fill_mesh_vertices(maps, mesh, &res);
        fill_mesh_normals(maps, mesh, &res);
        fill_mesh_texcoords(maps, mesh, &res);
        fill_mesh_materials(maps, mesh, &res);
        fill_mesh_images(maps, mesh, &res);

        fill_mesh_facets(maps, mesh, &res);
        fill_mesh_points(maps, mesh, &res);
        fill_mesh_lines(maps, mesh, &res);

        return res;
}

#define TEMPLATE(N) template Mesh<(N)> optimize(const Mesh<(N)>&);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
