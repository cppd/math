/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "mesh_hyperplane_simplex.h"

#include "../objects.h"
#include "../space/parallelotope_ortho.h"
#include "../space/tree.h"

#include <src/color/color.h>
#include <src/image/color_image.h>
#include <src/model/mesh_object.h>
#include <src/numerical/matrix.h>
#include <src/progress/progress.h>

#include <optional>

namespace painter
{
template <size_t N, typename T>
class MeshObject final
{
        using TreeParallelotope = ParallelotopeOrtho<N, T>;
        using Facet = MeshHyperplaneSimplex<N, T>;

        struct Material
        {
                Color Kd;
                Color::DataType diffuse;
                int map_Kd;
                Material(const Color& Kd, Color::DataType diffuse, int map_Kd)
                        : Kd(Kd), diffuse(diffuse), map_Kd(map_Kd)
                {
                }
        };

        std::vector<Vector<N, T>> m_vertices;
        std::vector<Vector<N, T>> m_normals;
        std::vector<Vector<N - 1, T>> m_texcoords;
        std::vector<Material> m_materials;
        std::vector<image::ColorImage<N - 1>> m_images;
        std::vector<Facet> m_facets;

        SpatialSubdivisionTree<TreeParallelotope> m_tree;
        Vector<N, T> m_min, m_max;

        static void create_tree(
                const std::vector<Facet>& facets,
                SpatialSubdivisionTree<TreeParallelotope>* tree,
                ProgressRatio* progress);

        void create(const mesh::Reading<N>& mesh_object);

public:
        MeshObject(const mesh::Reading<N>& mesh_object, ProgressRatio* progress);

        // Грани имеют адреса первых элементов векторов вершин,
        // нормалей и текстурных координат, поэтому при копировании
        // объекта надо менять адреса этих векторов в гранях.
        MeshObject(const MeshObject&) = delete;
        MeshObject(MeshObject&&) = delete;
        MeshObject& operator=(const MeshObject&) = delete;
        MeshObject& operator=(MeshObject&&) = delete;

        bool intersect_approximate(const Ray<N, T>& r, T* t) const;
        bool intersect_precise(const Ray<N, T>&, T approximate_t, T* t, const void** intersection_data) const;

        SurfaceProperties<N, T> surface_properties(const Vector<N, T>& p, const void* intersection_data) const;

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const;
};
}
