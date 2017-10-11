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

#include "barycentric.h"

#include "path_tracing/constants.h"

namespace
{
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
}

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

// Точка находится в треугольнике, если все барицентрические координаты больше 0
bool triangle_intersect(const ray3& ray, const vec3& normal, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, double* t)
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
bool rectangle_intersect(const ray3& ray, const vec3& normal, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, double* t)
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
