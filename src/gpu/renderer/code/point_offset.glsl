/*
Copyright (C) 2017-2024 Topological Manifold

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

#ifndef POINT_OFFSET_GLSL
#define POINT_OFFSET_GLSL

#include "constant.glsl"

const float POINT_OFFSET = 64 * FLOAT_EPSILON;

vec3 offset_ray_org(const vec3 geometric_normal, const vec3 ray_org, const vec3 ray_dir)
{
        const float ray_offset = dot(geometric_normal, ray_dir) < 0 ? -POINT_OFFSET : POINT_OFFSET;
        return ray_org + ray_offset * geometric_normal * abs(ray_org);
}

#endif
