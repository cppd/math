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

#include "triangle.h"

#include "barycentric.h"

#include "com/error.h"

TableTriangle::TableTriangle(const vec3* points, const vec3* normals, const vec2* texcoords, int v0, int v1, int v2,
                             bool has_normals, int n0, int n1, int n2, bool has_texcoords, int t0, int t1, int t2, int material)
{
        ASSERT((has_normals && n0 >= 0 && n1 >= 0 && n2 >= 0) || !has_normals);
        ASSERT((has_texcoords && t0 >= 0 && t1 >= 0 && t2 >= 0) || !has_texcoords);

        m_v0 = v0;
        m_v1 = v1;
        m_v2 = v2;

        if (has_texcoords)
        {
                m_t0 = t0;
                m_t1 = t1;
                m_t2 = t2;
        }
        else
        {
                m_t0 = -1;
        }

        m_material = material;

        m_vertices = points;
        m_normals = normals;
        m_texcoords = texcoords;

        m_normal = normalize(cross(m_vertices[m_v1] - m_vertices[m_v0], m_vertices[m_v2] - m_vertices[m_v0]));

        triangle_u_beta_and_u_gamma_for_v0(m_vertices[m_v0], m_vertices[m_v1], m_vertices[m_v2], &m_u_beta, &m_u_gamma);

        if (!has_normals)
        {
                m_normal_type = NormalType::NO_NORMALS;
                return;
        }

        m_n0 = n0;
        m_n1 = n1;
        m_n2 = n2;

        double d0 = dot(m_normals[m_n0], m_normal);
        double d1 = dot(m_normals[m_n1], m_normal);
        double d2 = dot(m_normals[m_n2], m_normal);

        constexpr double LIMIT_COSINE = 0.7; // 0.7 немного больше 45 градусов

        if (std::abs(d0) < LIMIT_COSINE || std::abs(d1) < LIMIT_COSINE || std::abs(d2) < LIMIT_COSINE)
        {
                // «Перпендикуляры» на вершинах совсем не перпендикуляры,
                // поэтому треугольник считать плоским.
                m_normal_type = NormalType::NO_NORMALS;
                return;
        }

        if (d0 > 0 && d1 > 0 && d2 > 0)
        {
                // Реальный перпендикуляр и «перпендикуляры» вершин имеют
                // одинаковое направление, поэтому оставить как есть.
                m_normal_type = NormalType::USE_NORMALS;
                return;
        }

        if (d0 < 0 && d1 < 0 && d2 < 0)
        {
                // Реальный перпендикуляр и «перпендикуляры» вершин имеют
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
        m_negate_normal_0 = d0 < 0;
        m_negate_normal_1 = d1 < 0;
        m_negate_normal_2 = d2 < 0;
}

bool TableTriangle::intersect(const ray3& r, double* t) const
{
        return triangle_intersect(r, m_normal, m_vertices[m_v0], m_u_beta, m_u_gamma, t);
}

vec3 TableTriangle::geometric_normal() const
{
        return m_normal;
}

vec3 TableTriangle::shading_normal(const vec3& point) const
{
        switch (m_normal_type)
        {
        case NormalType::NO_NORMALS:
                return m_normal;
        case NormalType::USE_NORMALS:
                return normalize(triangle_interpolation(point, m_vertices[m_v0], m_u_beta, m_u_gamma, m_normals[m_n0],
                                                        m_normals[m_n1], m_normals[m_n2]));

        case NormalType::NEGATE_NORMALS:
                return normalize(triangle_interpolation(point, m_vertices[m_v0], m_u_beta, m_u_gamma,
                                                        m_negate_normal_0 ? -m_normals[m_n0] : m_normals[m_n0],
                                                        m_negate_normal_1 ? -m_normals[m_n1] : m_normals[m_n1],
                                                        m_negate_normal_2 ? -m_normals[m_n2] : m_normals[m_n2]));
        }
        ASSERT(false);
}

bool TableTriangle::has_texcoord() const
{
        return m_t0 >= 0;
}

vec2 TableTriangle::texcoord(const vec3& point) const
{
        if (m_t0 >= 0)
        {
                return triangle_interpolation(point, m_vertices[m_v0], m_u_beta, m_u_gamma, m_texcoords[m_t0], m_texcoords[m_t1],
                                              m_texcoords[m_t2]);
        }
        ASSERT(false);
}

int TableTriangle::get_material() const
{
        return m_material;
}

const vec3& TableTriangle::v0() const
{
        return m_vertices[m_v0];
}

const vec3& TableTriangle::v1() const
{
        return m_vertices[m_v1];
}

const vec3& TableTriangle::v2() const
{
        return m_vertices[m_v2];
}
