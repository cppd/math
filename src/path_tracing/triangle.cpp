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

// Формулы имеются в книге
// Samuel R. Buss.
// 3D Computer Graphics. A Mathematical Introduction with OpenGL.
// Cambridge University Press, 2003.

#include "triangle.h"

#include "constants.h"

namespace
{
// Барицентрические координаты определяются при помощи
// u_beta и u_gamma по формулам IV.15 и IV.16.
// beta (v1) = dot(u_beta, point - v0)
// gamma (v2) = dot(u_gamma, point - v0)
// alpha (v0) = 1 - beta - gamma

void triangle_u_beta_and_u_gamma_for_v0(const vec3& v0, const vec3& v1, const vec3& v2, vec3* u_beta, vec3* u_gamma)
{
        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;

        double a = dot(e1, e1);
        double b = dot(e1, e2);
        double c = dot(e2, e2);
        double D = (a * c - b * b);

        *u_beta = (c * e1 - b * e2) / D;
        *u_gamma = (a * e2 - b * e1) / D;
}

vec3 triangle_barycentric_coordinates(const vec3& point, const vec3& v0, const vec3& u_beta, const vec3& u_gamma)
{
        vec3 r = point - v0;
        double beta = dot(u_beta, r);
        double gamma = dot(u_gamma, r);
        double alpha = 1 - beta - gamma;
        return vec3(alpha, beta, gamma);
}

bool plane_intersect(const ray3& ray, const vec3& plane_point, const vec3& normal, double* t)
{
        double c = dot(normal, ray.get_dir());
        if (std::abs(c) < EPSILON)
        {
                return false;
        }

        *t = dot(plane_point - ray.get_org(), normal) / c;
        if (*t < INTERSECTION_THRESHOLD)
        {
                return false;
        }

        return true;
}

// Точка находится в треугольнике, если все барицентрические координаты больше 0
bool triangle_intersect(const ray3& ray, const vec3& v0, const vec3& normal, const vec3& u_beta, const vec3& u_gamma, double* t)
{
        if (!plane_intersect(ray, v0, normal, t))
        {
                return false;
        }

        vec3 r = ray.point(*t) - v0;

        double beta = dot(u_beta, r);
        if (beta <= 0)
        {
                return false;
        }

        double gamma = dot(u_gamma, r);
        if (gamma <= 0)
        {
                return false;
        }

        double alpha = 1 - beta - gamma;

        return alpha > 0;
}

// Точка находится в прямоугольнике, если 2 барицентрические координаты больше 0 и меньше 1
bool rectangle_intersect(const ray3& ray, const vec3& v0, const vec3& normal, const vec3& u_beta, const vec3& u_gamma, double* t)
{
        if (!plane_intersect(ray, v0, normal, t))
        {
                return false;
        }

        vec3 r = ray.point(*t) - v0;

        double beta = dot(u_beta, r);
        if (beta <= 0 || beta >= 1)
        {
                return false;
        }

        double gamma = dot(u_gamma, r);
        if (gamma <= 0 || gamma >= 1)
        {
                return false;
        }

        return true;
}

vec3 triangle_normal_at_point(const vec3& point, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, const vec3& n0,
                              const vec3& n1, const vec3& n2)
{
        vec3 bc = triangle_barycentric_coordinates(point, v0, u_beta, u_gamma);
        return normalize(bc[0] * n0 + bc[1] * n1 + bc[2] * n2);
}

vec2 triangle_texcoord_at_point(const vec3& point, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, const vec2& t0,
                                const vec2& t1, const vec2& t2)
{
        vec3 bc = triangle_barycentric_coordinates(point, v0, u_beta, u_gamma);
        return bc[0] * t0 + bc[1] * t1 + bc[2] * t2;
}
}

//
// TableTriangle
//

void TableTriangle::set_data(const vec3* points, const vec3* normals, const vec2* texcoords, int v0, int v1, int v2, int n0,
                             int n1, int n2, int t0, int t1, int t2, int material)
{
        m_v0 = v0;
        m_v1 = v1;
        m_v2 = v2;

        m_n0 = n0;
        m_n1 = n1;
        m_n2 = n2;

        m_t0 = t0;
        m_t1 = t1;
        m_t2 = t2;

        m_material = material;

        m_points = points;
        m_normals = normals;
        m_texcoords = texcoords;

        m_normal = normalize(cross(m_points[m_v1] - m_points[m_v0], m_points[m_v2] - m_points[m_v0]));

        triangle_u_beta_and_u_gamma_for_v0(m_points[m_v0], m_points[m_v1], m_points[m_v2], &m_u_beta, &m_u_gamma);
}

bool TableTriangle::intersect(const ray3& r, double* t) const
{
        return triangle_intersect(r, m_points[m_v0], m_normal, m_u_beta, m_u_gamma, t);
}

vec3 TableTriangle::normal(const vec3& point) const
{
        return triangle_normal_at_point(point, m_points[m_v0], m_u_beta, m_u_gamma, m_normals[m_n0], m_normals[m_n1],
                                        m_normals[m_n2]);
}

vec2 TableTriangle::texcoord(const vec3& point) const
{
        return triangle_texcoord_at_point(point, m_points[m_v0], m_u_beta, m_u_gamma, m_texcoords[m_t0], m_texcoords[m_t1],
                                          m_texcoords[m_t2]);
}

int TableTriangle::get_material() const
{
        return m_material;
}

const vec3& TableTriangle::v0() const
{
        return m_points[m_v0];
}

const vec3& TableTriangle::v1() const
{
        return m_points[m_v1];
}

const vec3& TableTriangle::v2() const
{
        return m_points[m_v2];
}

//
// Rectangle
//

Rectangle::Rectangle(const vec3& org, const vec3& e0, const vec3& e1)
{
        set_data(org, e0, e1);
}

void Rectangle::set_data(const vec3& org, const vec3& e0, const vec3& e1)
{
        m_org = org;
        m_e0 = e0;
        m_e1 = e1;

        m_normal = normalize(cross(e0, e1));

        triangle_u_beta_and_u_gamma_for_v0(m_org, m_org + m_e0, m_org + m_e1, &m_u_beta, &m_u_gamma);
}

bool Rectangle::intersect(const ray3& r, double* t) const
{
        return rectangle_intersect(r, m_org, m_normal, m_u_beta, m_u_gamma, t);
}
vec3 Rectangle::normal(const vec3&) const
{
        return m_normal;
}

const vec3& Rectangle::org() const
{
        return m_org;
}
const vec3& Rectangle::e0() const
{
        return m_e0;
}
const vec3& Rectangle::e1() const
{
        return m_e1;
}
