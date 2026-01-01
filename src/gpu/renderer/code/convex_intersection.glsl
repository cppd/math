/*
Copyright (C) 2017-2026 Topological Manifold

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

#ifndef CONVEX_INTERSECTION_GLSL
#define CONVEX_INTERSECTION_GLSL

bool box_intersection(
        const vec3 org,
        const vec3 dir,
        const vec3 box_min,
        const vec3 box_max,
        out float near,
        out float far)
{
        near = 0;
        far = 1e38;
        for (int i = 0; i < 3; ++i)
        {
                const float s = dir[i];
                const float d = org[i];
                if (s == 0)
                {
                        if (d < box_min[i] || d > box_max[i])
                        {
                                return false;
                        }
                        continue;
                }
                const bool dir_negative = (s < 0);
                const float r = 1 / s;
                const vec2 a = vec2((box_min[i] - d) * r, (box_max[i] - d) * r);
                near = max(near, a[int(dir_negative)]);
                far = min(far, a[int(!dir_negative)]);
                if (far < near)
                {
                        return false;
                }
        }
        return true;
}

bool plane_intersection(const vec3 org, const vec3 dir, const vec4 plane, inout float near, inout float far)
{
        const vec3 plane_n = plane.xyz;
        const float plane_d = plane.w;
        const float s = dot(dir, plane_n);
        const float d = dot(org, plane_n);
        if (s == 0)
        {
                if (d > plane_d)
                {
                        return false;
                }
                return true;
        }
        const float a = (plane_d - d) / s;
        if (s > 0)
        {
                far = min(a, far);
        }
        else
        {
                near = max(a, near);
        }
        if (far < near)
        {
                return false;
        }
        return true;
}

#endif
