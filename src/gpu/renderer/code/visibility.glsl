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

#ifndef VISIBILITY_GLSL
#define VISIBILITY_GLSL

#ifdef RAY_TRACING

#extension GL_EXT_ray_query : require

const float RAY_TRACING_FLOAT_EPSILON = 1.0 / (1 << 23);
const float RAY_TRACING_RAY_OFFSET = 64 * RAY_TRACING_FLOAT_EPSILON;

const float RAY_TRACING_T_MAX = 1e38;

vec3 ray_tracing_move_ray_org(const vec3 geometric_normal, const vec3 ray_org, const vec3 ray_dir)
{
        const float ray_offset = dot(geometric_normal, ray_dir) < 0 ? -RAY_TRACING_RAY_OFFSET : RAY_TRACING_RAY_OFFSET;
        return ray_org + ray_offset * geometric_normal * abs(ray_org);
}

bool ray_tracing_intersection(
        const vec3 org,
        const vec3 dir,
        const accelerationStructureEXT acceleration_structure,
        const float t_max)
{
        const uint flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;

        rayQueryEXT ray_query;
        rayQueryInitializeEXT(ray_query, acceleration_structure, flags, /*cullMask*/ 0xFF, org, /*tMin*/ 0, dir, t_max);
        rayQueryProceedEXT(ray_query);

        return rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

bool occluded(
        const vec3 org,
        const vec3 dir,
        const vec3 geometric_normal,
        const accelerationStructureEXT acceleration_structure)
{
        const vec3 moved_org = ray_tracing_move_ray_org(geometric_normal, org, dir);

        return ray_tracing_intersection(moved_org, dir, acceleration_structure, RAY_TRACING_T_MAX);
}

bool occluded(
        const vec3 org,
        const vec3 dir,
        const vec3 geometric_normal,
        const vec4 clip_plane_equation,
        const accelerationStructureEXT acceleration_structure)
{
        const vec3 moved_org = ray_tracing_move_ray_org(geometric_normal, org, dir);

        // org is inside the clip plane

        const float n_dot_dir = dot(clip_plane_equation.xyz, dir);
        if (n_dot_dir >= 0)
        {
                return ray_tracing_intersection(moved_org, dir, acceleration_structure, RAY_TRACING_T_MAX);
        }

        const float t_clip_plane = dot(clip_plane_equation, vec4(moved_org, 1)) / -n_dot_dir;
        if (t_clip_plane > 0)
        {
                const float t_max = min(RAY_TRACING_T_MAX, t_clip_plane);
                return ray_tracing_intersection(moved_org, dir, acceleration_structure, t_max);
        }

        return false;
}

#endif

#endif
