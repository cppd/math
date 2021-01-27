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

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/geometry/spatial/constraint.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/orthogonal.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <string>
#include <vector>

namespace ns::painter::shapes
{
namespace mesh_facet_implementation
{
template <std::size_t N, typename T>
std::string vertices_to_string(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& v)
{
        std::string res;
        for (unsigned i = 0; i < N; ++i)
        {
                res += to_string(vertices[v[i]]);
                if (i != N - 1)
                {
                        res += '\n';
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> vertices_to_array(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& v)
{
        std::array<Vector<N, T>, N> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = vertices[v[i]];
        }
        return res;
}
}

template <std::size_t N, typename T>
class MeshFacet
{
        static_assert(N >= 3);

        // Минимум абсолютного значения косинуса угла между нормалью симплекса и нормалями
        // его вершин, при котором используются эти нормали вершин. При меньших значениях
        // косинуса нормали вершин считаются неправильными и игнорируются.
        static constexpr T LIMIT_COSINE = 0.7; // 0.7 немного больше 45 градусов

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

        geometry::HyperplaneSimplex<N, T> m_geometry;

        enum class NormalType : char
        {
                None,
                Use,
                Reverse
        } m_normal_type;

        std::array<bool, N> m_reverse_normal;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

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
                int material)
                : m_vertices(vertices), m_normals(normals), m_texcoords(texcoords)
        {
                ASSERT((has_normals && all_non_negative(normal_indices)) || !has_normals);
                ASSERT((has_texcoords && all_non_negative(texcoord_indices)) || !has_texcoords);

                m_v = vertex_indices;

                if (has_texcoords)
                {
                        m_t = texcoord_indices;
                }
                else
                {
                        m_t[0] = -1;
                }

                m_material = material;

                m_normal = numerical::ortho_nn(m_vertices, m_v).normalized();

                if (!is_finite(m_normal))
                {
                        error("Mesh facet normal is not finite, facet vertices\n"
                              + mesh_facet_implementation::vertices_to_string(m_vertices, m_v));
                }

                m_geometry.set_data(m_normal, mesh_facet_implementation::vertices_to_array(m_vertices, m_v));

                if (!has_normals)
                {
                        m_normal_type = NormalType::None;
                        return;
                }

                m_n = normal_indices;

                std::array<T, N> dots;

                for (unsigned i = 0; i < N; ++i)
                {
                        dots[i] = dot(m_normals[m_n[i]], m_normal);
                }

                if (!std::all_of(
                            dots.cbegin(), dots.cend(),
                            [](const T& d)
                            {
                                    static_assert(LIMIT_COSINE > 0);
                                    return is_finite(d) && std::abs(d) >= LIMIT_COSINE;
                            }))
                {
                        // «Перпендикуляры» на вершинах совсем не перпендикуляры,
                        // поэтому симплекс считать плоским.
                        m_normal_type = NormalType::None;
                        return;
                }

                if (all_positive(dots))
                {
                        // Реальный перпендикуляр и «перпендикуляры» вершин имеют
                        // одинаковое направление, поэтому оставить как есть.
                        m_normal_type = NormalType::Use;
                        return;
                }

                if (all_negative(dots))
                {
                        // Реальный перпендикуляр и все «перпендикуляры» вершин имеют
                        // противоположное направление, поэтому поменять направление
                        // реального перпендикуляра.
                        m_normal_type = NormalType::Use;
                        m_normal = -m_normal;
                        return;
                }

                // «Перпендикуляры» на вершинах могут быть направлены в разные стороны от грани.
                // Это происходит, например, при восстановлении поверхностей по алгоритмам
                // типа Cocone, где соседние объекты Вороного имеют положительные полюсы
                // в противоположных направлениях.
                m_normal_type = NormalType::Reverse;
                for (unsigned i = 0; i < N; ++i)
                {
                        m_reverse_normal[i] = dots[i] < 0;
                }
        }

        int material() const
        {
                return m_material;
        }

        bool has_texcoord() const
        {
                return m_t[0] >= 0;
        }

        Vector<N - 1, T> texcoord(const Vector<N, T>& point) const
        {
                if (has_texcoord())
                {
                        std::array<Vector<N - 1, T>, N> texcoords;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                texcoords[i] = m_texcoords[m_t[i]];
                        }
                        return m_geometry.interpolate(point, texcoords);
                }
                error("Mesh facet texture coordinates request when there are no texture coordinates");
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return m_geometry.intersect(r, m_vertices[m_v[0]], m_normal);
        }

        Vector<N, T> geometric_normal() const
        {
                return m_normal;
        }

        Vector<N, T> shading_normal(const Vector<N, T>& point) const
        {
                switch (m_normal_type)
                {
                case NormalType::None:
                {
                        return m_normal;
                }
                case NormalType::Use:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = m_normals[m_n[i]];
                        }
                        return m_geometry.interpolate(point, normals).normalized();
                }
                case NormalType::Reverse:
                {
                        std::array<Vector<N, T>, N> normals;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                normals[i] = m_reverse_normal[i] ? -m_normals[m_n[i]] : m_normals[m_n[i]];
                        }
                        return m_geometry.interpolate(point, normals).normalized();
                }
                }
                error_fatal("Unknown mesh facet normal type");
        }

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const
        {
                return mesh_facet_implementation::vertices_to_array(m_vertices, m_v);
        }

        geometry::Constraints<N, T, N, 1> constraints() const
        {
                return m_geometry.constraints(m_normal, mesh_facet_implementation::vertices_to_array(m_vertices, m_v));
        }

        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> vertex_ridges() const
        {
                std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> result;
                unsigned n = 0;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                result[n++] = {m_vertices[m_v[i]], m_vertices[m_v[j]] - m_vertices[m_v[i]]};
                        }
                }
                ASSERT(n == VERTEX_RIDGE_COUNT);
                return result;
        }
};
}
