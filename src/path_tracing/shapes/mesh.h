/*
Copyright (C) 2017 Topological Manifold

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

#include "parallelepiped_ortho.h"
#include "triangle.h"

#include "com/mat.h"
#include "obj/obj.h"
#include "path_tracing/image/image.h"
#include "path_tracing/objects.h"
#include "path_tracing/octree.h"
#include "progress/progress.h"

#include <optional>

class Mesh final
{
        using OctreeParallelepiped = ParallelepipedOrtho;

        std::vector<vec3> m_vertices;
        std::vector<vec3> m_normals;
        std::vector<vec2> m_texcoords;
        struct Material
        {
                vec3 Kd, Ks;
                double Ns;
                int map_Kd, map_Ks;
                Material(const vec3& Kd_, const vec3& Ks_, double Ns_, int map_Kd_, int map_Ks_)
                        : Kd(Kd_), Ks(Ks_), Ns(Ns_), map_Kd(map_Kd_), map_Ks(map_Ks_)
                {
                }
        };
        std::vector<Material> m_materials;
        std::vector<Image> m_images;

        std::vector<TableTriangle> m_triangles;

        Octree<OctreeParallelepiped> m_octree;

        void create_mesh_object(const IObj* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress);

public:
        Mesh(const IObj* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress);

        ~Mesh() = default;

        // Треугольники имеют адреса первых элементов векторов вершин, нормалей и текстурных координат,
        // поэтому при копировании объекта надо менять адреса этих векторов в треугольниках.
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        bool intersect_approximate(const ray3& r, double* t) const;
        bool intersect_precise(const ray3&, double approximate_t, double* t, const GeometricObject** geometric_object) const;

        vec3 get_geometric_normal(const GeometricObject* geometric_object) const;
        vec3 get_shading_normal(const vec3& p, const GeometricObject* geometric_object) const;

        std::optional<vec3> get_color(const vec3& p, const GeometricObject* geometric_object) const;
};
