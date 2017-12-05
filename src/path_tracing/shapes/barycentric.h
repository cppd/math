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

#pragma once

#include "com/ray.h"
#include "com/vec.h"

void triangle_u_beta_and_u_gamma_for_v0(const vec3& v0, const vec3& v1, const vec3& v2, vec3* u_beta, vec3* u_gamma);

vec3 triangle_barycentric_coordinates(const vec3& point, const vec3& v0, const vec3& u_beta, const vec3& u_gamma);

bool triangle_intersect(const ray3& ray, const vec3& normal, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, double* t);

bool rectangle_intersect(const ray3& ray, const vec3& normal, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, double* t);

template <typename T>
T triangle_interpolation(const vec3& point, const vec3& v0, const vec3& u_beta, const vec3& u_gamma, const T& n0, const T& n1,
                         const T& n2)
{
        vec3 bc = triangle_barycentric_coordinates(point, v0, u_beta, u_gamma);
        return bc[0] * n0 + bc[1] * n1 + bc[2] * n2;
}
