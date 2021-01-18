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
#include "shading.h"
#include "shape.h"

#include "../objects.h"
#include "../space/parallelotope_aa.h"
#include "../space/tree.h"

#include <src/color/color.h>
#include <src/model/mesh_object.h>
#include <src/numerical/matrix.h>
#include <src/progress/progress.h>

#include <optional>

namespace ns::painter::shapes
{
template <std::size_t N, typename T>
class Mesh final : public Shape<N, T>, public Surface<N, T>
{
        using TreeParallelotope = ParallelotopeAA<N, T>;

        struct Material
        {
                T metalness;
                T roughness;
                Color Kd;
                int map_Kd;
                Material(T metalness, T roughness, const Color& Kd, int map_Kd)
                        : metalness(std::clamp(metalness, T(0), T(1))),
                          roughness(std::clamp(roughness, T(0), T(1))),
                          Kd(Kd),
                          map_Kd(map_Kd)
                {
                }
        };

        std::vector<Vector<N, T>> m_vertices;
        std::vector<Vector<N, T>> m_normals;
        std::vector<Vector<N - 1, T>> m_texcoords;
        std::vector<Material> m_materials;
        std::vector<MeshTexture<N - 1>> m_images;
        std::vector<MeshFacet<N, T>> m_facets;

        SpatialSubdivisionTree<TreeParallelotope> m_tree;
        BoundingBox<N, T> m_bounding_box;

        Color::DataType m_alpha;

        Shading<N, T> m_shading;

        void create(const mesh::Reading<N>& mesh_object);
        void create(const std::vector<mesh::Reading<N>>& mesh_objects);

public:
        Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* progress);

        // Грани имеют адреса первых элементов векторов вершин,
        // нормалей и текстурных координат, поэтому при копировании
        // объекта надо менять адреса этих векторов в гранях.
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override;
        std::optional<Intersection<N, T>> intersect(const Ray<N, T>&, T bounding_distance) const override;

        SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* intersection_data) const override;

        Color lighting(
                const Vector<N, T>& p,
                const void* intersection_data,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override;

        SurfaceReflection<N, T> reflection(
                RandomEngine<T>& random_engine,
                const Vector<N, T>& p,
                const void* intersection_data,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override;

        BoundingBox<N, T> bounding_box() const override;
        std::function<bool(const ShapeWrapperForIntersection<painter::ParallelotopeAA<N, T>>&)> intersection_function()
                const override;
};
}
