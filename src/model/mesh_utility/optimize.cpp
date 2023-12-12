/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "position.h"

#include "../mesh.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <vector>

namespace ns::model::mesh
{
namespace
{
template <std::size_t N>
using MeshFacet = Mesh<N>::Facet;

template <std::size_t N>
using MeshPoint = Mesh<N>::Point;

template <std::size_t N>
using MeshLine = Mesh<N>::Line;

template <std::size_t N>
using MeshMaterial = Mesh<N>::Material;

//

bool map_insert(const int index, std::vector<int>* const map, int* const idx)
{
        if (index < 0 || static_cast<std::size_t>(index) >= map->size())
        {
                error("Mesh index " + to_string(index) + " is out of bounds [0, " + to_string(map->size()) + ")");
        }
        int& v = (*map)[index];
        if (v >= 0)
        {
                return false;
        }
        v = *idx;
        ++(*idx);
        return true;
}

int map_index(const std::vector<int>& map, const int index)
{
        ASSERT(index >= 0 && static_cast<std::size_t>(index) < map.size());
        const int res = map[index];
        ASSERT(res >= 0);
        return res;
}

template <std::size_t N>
void insert_vertices(const MeshFacet<N>& facet, std::vector<int>* const vertices, int* const idx_vertices)
{
        for (const int index : facet.vertices)
        {
                map_insert(index, vertices, idx_vertices);
        }
}

template <std::size_t N>
void insert_vertices(const MeshPoint<N>& point, std::vector<int>* const vertices, int* const idx_vertices)
{
        map_insert(point.vertex, vertices, idx_vertices);
}

template <std::size_t N>
void insert_vertices(const MeshLine<N>& line, std::vector<int>* const vertices, int* const idx_vertices)
{
        for (const int index : line.vertices)
        {
                map_insert(index, vertices, idx_vertices);
        }
}

template <std::size_t N>
void insert_normals(const MeshFacet<N>& facet, std::vector<int>* const normals, int* const idx_normals)
{
        if (!facet.has_normal)
        {
                return;
        }
        for (const int index : facet.normals)
        {
                map_insert(index, normals, idx_normals);
        }
}

template <std::size_t N>
void insert_texcoords(const MeshFacet<N>& facet, std::vector<int>* const texcoords, int* const idx_texcoords)
{
        if (!facet.has_texcoord)
        {
                return;
        }
        for (const int index : facet.texcoords)
        {
                map_insert(index, texcoords, idx_texcoords);
        }
}

template <std::size_t N>
void insert_materials_and_images(
        const MeshFacet<N>& facet,
        const std::vector<MeshMaterial<N>>& mesh_materials,
        std::vector<int>* const materials,
        int* const idx_materials,
        std::vector<int>* const images,
        int* const idx_images)
{
        if (facet.material < 0)
        {
                return;
        }
        if (!map_insert(facet.material, materials, idx_materials))
        {
                return;
        }
        const int image = mesh_materials[facet.material].image;
        if (image < 0)
        {
                return;
        }
        map_insert(image, images, idx_images);
}

//

struct Maps final
{
        std::vector<int> vertices;
        std::vector<int> normals;
        std::vector<int> texcoords;
        std::vector<int> materials;
        std::vector<int> images;
        int vertex_count;
        int normal_count;
        int texcoord_count;
        int material_count;
        int image_count;
};

template <std::size_t N>
Maps create_maps(const Mesh<N>& mesh)
{
        Maps res;

        res.vertices.resize(mesh.vertices.size(), -1);
        res.normals.resize(mesh.normals.size(), -1);
        res.texcoords.resize(mesh.texcoords.size(), -1);
        res.materials.resize(mesh.materials.size(), -1);
        res.images.resize(mesh.images.size(), -1);

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

        res.vertex_count = idx_vertices;
        res.normal_count = idx_normals;
        res.texcoord_count = idx_texcoords;
        res.material_count = idx_materials;
        res.image_count = idx_images;

        return res;
}

//

template <std::size_t N>
void fill_facet_vertices(const Maps& maps, const MeshFacet<N>& src_facet, MeshFacet<N>* const facet)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                facet->vertices[i] = map_index(maps.vertices, src_facet.vertices[i]);
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
                facet->normals[i] = map_index(maps.normals, src_facet.normals[i]);
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
                facet->texcoords[i] = map_index(maps.texcoords, src_facet.texcoords[i]);
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

        facet->material = map_index(maps.materials, src_facet.material);
}

//

template <std::size_t N>
void fill_mesh_vertices(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->vertices.resize(maps.vertex_count);
        const std::vector<int>& map = maps.vertices;
        ASSERT(map.size() == src_mesh.vertices.size());
        for (std::size_t i = 0, size = map.size(); i < size; ++i)
        {
                const int new_index = map[i];
                if (new_index >= 0)
                {
                        ASSERT(static_cast<std::size_t>(new_index) < mesh->vertices.size());
                        mesh->vertices[new_index] = src_mesh.vertices[i];
                }
        }
}

template <std::size_t N>
void fill_mesh_normals(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->normals.resize(maps.normal_count);
        const std::vector<int>& map = maps.normals;
        ASSERT(map.size() == src_mesh.normals.size());
        for (std::size_t i = 0, size = map.size(); i < size; ++i)
        {
                const int new_index = map[i];
                if (new_index >= 0)
                {
                        ASSERT(static_cast<std::size_t>(new_index) < mesh->normals.size());
                        mesh->normals[new_index] = src_mesh.normals[i];
                }
        }
}

template <std::size_t N>
void fill_mesh_texcoords(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->texcoords.resize(maps.texcoord_count);
        const std::vector<int>& map = maps.texcoords;
        ASSERT(map.size() == src_mesh.texcoords.size());
        for (std::size_t i = 0, size = map.size(); i < size; ++i)
        {
                const int new_index = map[i];
                if (new_index >= 0)
                {
                        ASSERT(static_cast<std::size_t>(new_index) < mesh->texcoords.size());
                        mesh->texcoords[new_index] = src_mesh.texcoords[i];
                }
        }
}

template <std::size_t N>
void fill_mesh_materials(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        ASSERT(maps.materials.size() == src_mesh.materials.size());
        ASSERT(maps.images.size() == src_mesh.images.size());
        mesh->materials.resize(maps.material_count);
        for (std::size_t i = 0, size = maps.materials.size(); i < size; ++i)
        {
                const int new_material_index = maps.materials[i];
                if (new_material_index < 0)
                {
                        continue;
                }

                ASSERT(static_cast<std::size_t>(new_material_index) < mesh->materials.size());
                mesh->materials[new_material_index] = src_mesh.materials[i];

                const int old_image_index = src_mesh.materials[i].image;
                if (old_image_index < 0)
                {
                        continue;
                }
                const int new_image_index = maps.images[old_image_index];
                ASSERT(new_image_index >= 0);
                ASSERT(new_image_index < maps.image_count);
                mesh->materials[new_material_index].image = new_image_index;
        }
}

template <std::size_t N>
void fill_mesh_images(const Maps& maps, const Mesh<N>& src_mesh, Mesh<N>* const mesh)
{
        mesh->images.resize(maps.image_count);
        const std::vector<int>& map = maps.images;
        ASSERT(map.size() == src_mesh.images.size());
        for (std::size_t i = 0, size = map.size(); i < size; ++i)
        {
                const int new_index = map[i];
                if (new_index >= 0)
                {
                        ASSERT(static_cast<std::size_t>(new_index) < mesh->images.size());
                        mesh->images[new_index] = src_mesh.images[i];
                }
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
                p.vertex = map_index(maps.vertices, point.vertex);
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
                        l.vertices[i] = map_index(maps.vertices, line.vertices[i]);
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

        set_center_and_length(&res);

        return res;
}

#define TEMPLATE(N) template Mesh<(N)> optimize(const Mesh<(N)>&);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
