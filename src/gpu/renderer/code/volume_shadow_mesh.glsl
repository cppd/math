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

#ifndef VOLUME_SHADOW_MESH_GLSL
#define VOLUME_SHADOW_MESH_GLSL

#include "visibility.glsl"
#include "volume_in.glsl"

#ifdef RAY_TRACING

float mesh_shadow_transparency_world(const vec3 world_position, const vec3 geometric_normal)
{
        if (!drawing.clip_plane_enabled)
        {
                return occluded(world_position, drawing.direction_to_light, geometric_normal, acceleration_structure)
                               ? 0
                               : 1;
        }

        return occluded(
                       world_position, drawing.direction_to_light, geometric_normal, drawing.clip_plane_equation,
                       acceleration_structure)
                       ? 0
                       : 1;
}

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 world_position = volume_coordinates.texture_to_world_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency_world(world_position.xyz, /*geometric_normal=*/vec3(0));
}
#endif

#else

float mesh_shadow_transparency_shadow(const vec3 shadow_position)
{
        const float d = texture(shadow_mapping_texture, shadow_position.xy).x;
        return d <= shadow_position.z ? 0 : 1;
}

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 shadow_position = volume_coordinates.texture_to_shadow_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency_shadow(shadow_position.xyz);
}
#endif

#if defined(OPACITY) || defined(TRANSPARENCY)
float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 shadow_position = coordinates.device_to_shadow * vec4(device_position, 1);
        return mesh_shadow_transparency_shadow(shadow_position.xyz);
}
#endif

#endif

#endif
