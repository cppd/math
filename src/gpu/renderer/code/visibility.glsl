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

#include "ray_tracing_intersection.glsl"

#ifdef RAY_TRACING

#extension GL_EXT_ray_query : require

bool occluded(
        const vec3 world_position,
        const vec3 direction_to_light,
        const bool /*self_intersection*/,
        const accelerationStructureEXT acceleration_structure)
{
        const vec3 org = world_position;
        const vec3 dir = direction_to_light;
        const bool intersection = ray_tracing_intersection(org, dir, acceleration_structure);
        return intersection;
}

bool occluded(
        const vec3 world_position,
        const vec3 direction_to_light,
        const bool /*self_intersection*/,
        const vec4 clip_plane_equation,
        const accelerationStructureEXT acceleration_structure)
{
        const vec3 org = world_position;
        const vec3 dir = direction_to_light;
        const bool intersection = ray_tracing_intersection(org, dir, acceleration_structure, clip_plane_equation);
        return intersection;
}

#else

bool occluded(const vec3 shadow_position, const sampler2D shadow_mapping_texture)
{
        const float d = texture(shadow_mapping_texture, shadow_position.xy).x;
        return d <= shadow_position.z;
}

#endif

#endif
