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

#include "objects.h"

#include "image.h"
#include "octree.h"
#include "parallelepiped.h"
#include "triangle.h"

#include "obj/obj.h"

#include <memory>

class VisibleMesh final : public GenericObject, public Surface, public SurfaceProperties
{
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

        std::unique_ptr<Octree<Parallelepiped, TableTriangle>> m_octree;

public:
        VisibleMesh(const IObj* obj, double size, const vec3& position);

        // Интерфейс GenericObject
        bool intersect_approximate(const ray3& r, double* t) const override;
        bool intersect_precise(const ray3&, double approximate_t, double* t, const Surface** surface,
                               const GeometricObject** geometric_object) const override;

        // Интерфейс Surface
        SurfaceProperties properties(const vec3& p, const GeometricObject* /*geometric_object*/) const override;
};
