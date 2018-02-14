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

#include "triangle.h"

#include "com/error.h"
#include "geometry/core/linear_algebra.h"

// Минимум абсолютного значения косинуса угла между нормалью симплекса и нормалями
// его вершин, при котором используются эти нормали вершин. При меньших значениях
// косинуса нормали вершин считаются неправильными и игнорируются.
template <typename T>
constexpr T LIMIT_COSINE = 0.7; // 0.7 немного больше 45 градусов

namespace
{
template <size_t N, typename T>
bool all_non_negative(const std::array<T, N>& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const T& v) { return v >= 0; });
}

template <size_t N, typename T>
bool all_positive(const std::array<T, N>& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const T& v) { return v > 0; });
}

template <size_t N, typename T>
bool all_negative(const std::array<T, N>& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const T& v) { return v < 0; });
}

template <size_t N, typename T>
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

template <size_t N, typename T>
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

template <size_t N, typename T>
MeshSimplex<N, T>::MeshSimplex(const std::vector<Vector<N, T>>& vertices, const std::vector<Vector<N, T>>& normals,
                               const std::vector<Vector<N - 1, T>>& texcoords, const std::array<int, N>& vertex_indices,
                               bool has_normals, const std::array<int, N>& normal_indices, bool has_texcoords,
                               const std::array<int, N>& texcoord_indices, int material)
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

        m_normal = normalize(ortho_nn(m_vertices, m_v));

        if (!is_finite(m_normal))
        {
                error("Simplex normal is not finite, simplex vertices\n" + vertices_to_string(m_vertices, m_v));
        }

        m_geometry.set_data(m_normal, vertices_to_array(m_vertices, m_v));

        if (!has_normals)
        {
                m_normal_type = NormalType::NO_NORMALS;
                return;
        }

        m_n = normal_indices;

        std::array<T, N> dots;

        for (unsigned i = 0; i < N; ++i)
        {
                dots[i] = dot(m_normals[m_n[i]], m_normal);
        }

        if (std::any_of(dots.cbegin(), dots.cend(), [](const T& d) { return std::abs(d) < LIMIT_COSINE<T>; }))
        {
                // «Перпендикуляры» на вершинах совсем не перпендикуляры,
                // поэтому симплекс считать плоским.
                m_normal_type = NormalType::NO_NORMALS;
                return;
        }

        if (all_positive(dots))
        {
                // Реальный перпендикуляр и «перпендикуляры» вершин имеют
                // одинаковое направление, поэтому оставить как есть.
                m_normal_type = NormalType::USE_NORMALS;
                return;
        }

        if (all_negative(dots))
        {
                // Реальный перпендикуляр и все «перпендикуляры» вершин имеют
                // противоположное направление, поэтому поменять направление
                // реального перпендикуляра.
                m_normal_type = NormalType::USE_NORMALS;
                m_normal = -m_normal;
                return;
        }

        // «Перпендикуляры» на вершинах могут быть направлены в разные стороны от грани.
        // Это происходит, например, при восстановлении поверхностей по алгоритмам
        // типа COCONE, где соседние объекты Вороного имеют положительные полюсы
        // в противоположных направлениях.
        m_normal_type = NormalType::NEGATE_NORMALS;
        for (unsigned i = 0; i < N; ++i)
        {
                m_negate_normal[i] = dots[i] < 0;
        }
}

template <size_t N, typename T>
bool MeshSimplex<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        return m_geometry.intersect(r, m_vertices[m_v[0]], m_normal, t);
}

template <size_t N, typename T>
Vector<N, T> MeshSimplex<N, T>::geometric_normal() const
{
        return m_normal;
}

template <size_t N, typename T>
Vector<N, T> MeshSimplex<N, T>::shading_normal(const Vector<N, T>& point) const
{
        switch (m_normal_type)
        {
        case NormalType::NO_NORMALS:
        {
                return m_normal;
        }
        case NormalType::USE_NORMALS:
        {
                std::array<Vector<N, T>, N> normals;
                for (unsigned i = 0; i < N; ++i)
                {
                        normals[i] = m_normals[m_n[i]];
                }
                return normalize(m_geometry.interpolate(point, normals));
        }
        case NormalType::NEGATE_NORMALS:
        {
                std::array<Vector<N, T>, N> normals;
                for (unsigned i = 0; i < N; ++i)
                {
                        normals[i] = m_negate_normal[i] ? -m_normals[m_n[i]] : m_normals[m_n[i]];
                }
                return normalize(m_geometry.interpolate(point, normals));
        }
        }
        error_fatal("Unknown mesh simplex normal type");
}

template <size_t N, typename T>
bool MeshSimplex<N, T>::has_texcoord() const
{
        return m_t[0] >= 0;
}

template <size_t N, typename T>
Vector<N - 1, T> MeshSimplex<N, T>::texcoord(const Vector<N, T>& point) const
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
        error("Mesh simplex texture coordinates request when there are no texture coordinates");
}

template <size_t N, typename T>
int MeshSimplex<N, T>::material() const
{
        return m_material;
}

template <size_t N, typename T>
std::array<Vector<N, T>, N> MeshSimplex<N, T>::vertices() const
{
        return vertices_to_array(m_vertices, m_v);
}

template class MeshSimplex<3, double>;
template class MeshSimplex<4, double>;
template class MeshSimplex<5, double>;
