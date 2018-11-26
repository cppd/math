/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/color/color.h"
#include "com/matrix.h"
#include "obj/obj.h"
#include "painter/image/image.h"
#include "painter/space/parallelotope_ortho.h"
#include "painter/space/tree.h"
#include "progress/progress.h"

#include <optional>

template <size_t N, typename T>
class Mesh
{
        using TreeParallelotope = ParallelotopeOrtho<N, T>;
        using Facet = MeshHyperplaneSimplex<N, T>;

        std::vector<Vector<N, T>> m_vertices;
        std::vector<Vector<N, T>> m_normals;
        std::vector<Vector<N - 1, T>> m_texcoords;

        struct Material
        {
                Color Kd, Ks;
                Color::DataType Ns;
                int map_Kd, map_Ks;
                Material(const Color& Kd_, const Color& Ks_, Color::DataType Ns_, int map_Kd_, int map_Ks_)
                        : Kd(Kd_), Ks(Ks_), Ns(Ns_), map_Kd(map_Kd_), map_Ks(map_Ks_)
                {
                }
        };
        std::vector<Material> m_materials;

        std::vector<Image<N - 1>> m_images;

        std::vector<Facet> m_facets;

        SpatialSubdivisionTree<TreeParallelotope> m_tree;

        Vector<N, T> m_min, m_max;

        void create_mesh_object(const Obj<N>* obj, const Matrix<N + 1, N + 1, T>& vertex_matrix, unsigned thread_count,
                                ProgressRatio* progress);

public:
        Mesh(const Obj<N>* obj, const Matrix<N + 1, N + 1, T>& vertex_matrix, unsigned thread_count, ProgressRatio* progress);

        ~Mesh() = default;

        // Грани имеют адреса первых элементов векторов вершин,
        // нормалей и текстурных координат, поэтому при копировании
        // объекта надо менять адреса этих векторов в гранях.
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        bool intersect_approximate(const Ray<N, T>& r, T* t) const;
        bool intersect_precise(const Ray<N, T>&, T approximate_t, T* t, const void** intersection_data) const;

        Vector<N, T> geometric_normal(const void* intersection_data) const;
        Vector<N, T> shading_normal(const Vector<N, T>& p, const void* intersection_data) const;

        std::optional<Color> color(const Vector<N, T>& p, const void* intersection_data) const;

        void min_max(Vector<N, T>* min, Vector<N, T>* max) const;
};
