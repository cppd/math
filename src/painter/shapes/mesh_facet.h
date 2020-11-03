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

#include "../space/constraint.h"
#include "../space/hyperplane_simplex.h"

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <vector>

namespace painter::shapes
{
template <size_t N, typename T>
class MeshFacet
{
        static_assert(N >= 3);

        static constexpr int VERTEX_COUNT = N;

        // Количество сочетаний по 2 из N
        // N! / ((N - 2)! * 2!) = (N * (N - 1)) / 2
        static constexpr int VERTEX_RIDGE_COUNT = (N * (N - 1)) / 2;

        const std::vector<Vector<N, T>>& m_vertices;
        const std::vector<Vector<N, T>>& m_normals;
        const std::vector<Vector<N - 1, T>>& m_texcoords;

        std::array<int, N> m_v;
        std::array<int, N> m_n;
        std::array<int, N> m_t;

        int m_material;

        Vector<N, T> m_normal;

        HyperplaneSimplex<N, T> m_geometry;

        enum class NormalType : char
        {
                None,
                Use,
                Reverse
        } m_normal_type;

        std::array<bool, N> m_reverse_normal;

public:
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        MeshFacet(
                const std::vector<Vector<N, T>>& vertices,
                const std::vector<Vector<N, T>>& normals,
                const std::vector<Vector<N - 1, T>>& texcoords,
                const std::array<int, N>& vertex_indices,
                bool has_normals,
                const std::array<int, N>& normal_indices,
                bool has_texcoords,
                const std::array<int, N>& texcoord_indices,
                int material);

        int material() const;

        bool has_texcoord() const;
        Vector<N - 1, T> texcoord(const Vector<N, T>& point) const;

        bool intersect(const Ray<N, T>& r, T* t) const;

        Vector<N, T> geometric_normal() const;
        Vector<N, T> shading_normal(const Vector<N, T>& point) const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;
        Constraints<N, T, N, 1> constraints() const;
        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> vertex_ridges() const;
};
}
