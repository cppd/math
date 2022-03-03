/*
Copyright (C) 2017-2022 Topological Manifold

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

#ifndef RAY_TRACING_INTERSECTION_GLSL
#define RAY_TRACING_INTERSECTION_GLSL

#ifdef RAY_TRACING

#extension GL_EXT_ray_query : require

const float RAY_TRACING_T_MIN = 1e-5;
const float RAY_TRACING_T_MAX = 10;

bool ray_tracing_intersection(
        const vec3 org,
        const vec3 dir,
        const accelerationStructureEXT acceleration_structure,
        const float t_min,
        const float t_max)
{
        const uint flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;

        rayQueryEXT ray_query;
        rayQueryInitializeEXT(ray_query, acceleration_structure, flags, 0xFF, org, t_min, dir, t_max);
        rayQueryProceedEXT(ray_query);

        return rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

bool ray_tracing_intersection(const vec3 org, const vec3 dir, const accelerationStructureEXT acceleration_structure)
{
        return ray_tracing_intersection(org, dir, acceleration_structure, RAY_TRACING_T_MIN, RAY_TRACING_T_MAX);
}

bool ray_tracing_intersection(
        const vec3 org,
        const vec3 dir,
        const accelerationStructureEXT acceleration_structure,
        const vec4 clip_plane_equation)
{
        // org is inside the clip plane

        const float n_dot_dir = dot(clip_plane_equation.xyz, dir);
        if (n_dot_dir >= 0)
        {
                return ray_tracing_intersection(org, dir, acceleration_structure, RAY_TRACING_T_MIN, RAY_TRACING_T_MAX);
        }

        const float t_clip_plane = dot(clip_plane_equation, vec4(org, 1)) / -n_dot_dir;
        if (t_clip_plane > RAY_TRACING_T_MIN)
        {
                const float t_max = min(RAY_TRACING_T_MAX, t_clip_plane);
                return ray_tracing_intersection(org, dir, acceleration_structure, RAY_TRACING_T_MIN, t_max);
        }

        return false;
}

#endif

#endif
