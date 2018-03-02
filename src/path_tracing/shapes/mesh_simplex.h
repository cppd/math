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

#include "com/ray.h"
#include "com/vec.h"
#include "path_tracing/space/constraint.h"
#include "path_tracing/space/simplex_geometry.h"

#include <array>
#include <vector>

template <size_t N, typename T>
class MeshSimplex
{
        static_assert(N >= 3);

        const std::vector<Vector<N, T>>& m_vertices;
        const std::vector<Vector<N, T>>& m_normals;
        const std::vector<Vector<N - 1, T>>& m_texcoords;

        std::array<int, N> m_v;
        std::array<int, N> m_n;
        std::array<int, N> m_t;

        int m_material;

        Vector<N, T> m_normal;

        SimplexGeometry<N, T> m_geometry;

        enum class NormalType : char
        {
                NO_NORMALS,
                USE_NORMALS,
                NEGATE_NORMALS
        } m_normal_type;

        std::array<bool, N> m_negate_normal;

public:
        static constexpr size_t DIMENSION = N;
        using DataType = T;

        MeshSimplex(const std::vector<Vector<N, T>>& vertices, const std::vector<Vector<N, T>>& normals,
                    const std::vector<Vector<N - 1, T>>& texcoords, const std::array<int, N>& vertex_indices, bool has_normals,
                    const std::array<int, N>& normal_indices, bool has_texcoords, const std::array<int, N>& texcoord_indices,
                    int material);

        int material() const;

        bool has_texcoord() const;
        Vector<N - 1, T> texcoord(const Vector<N, T>& point) const;

        bool intersect(const Ray<N, T>& r, T* t) const;

        Vector<N, T> geometric_normal() const;
        Vector<N, T> shading_normal(const Vector<N, T>& point) const;

        std::array<Vector<N, T>, N> vertices() const;

        void constraints(std::array<Constraint<N, T>, N>* c, Constraint<N, T>* c_eq) const;
};
