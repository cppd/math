/*
Copyright (C) 2017-2025 Topological Manifold

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

#ifndef VOLUME_SHADOW_ISOSURFACE_GLSL
#define VOLUME_SHADOW_ISOSURFACE_GLSL

#include "volume_in.glsl"
#include "volume_intersect.glsl"

#if defined(IMAGE)

float isosurface_shadow_transparency_texture(const vec3 texture_position)
{
        const vec3 direction_to_light =
                normalize(mat3(volume_coordinates.world_to_texture_matrix) * drawing.direction_to_light);
        return isosurface_intersect(texture_position, direction_to_light) ? 0 : 1;
}

#if defined(OPACITY) || defined(TRANSPARENCY)

float isosurface_shadow_transparency_device(const vec3 device_position)
{
        const vec4 texture_position = volume_coordinates.device_to_texture_matrix * vec4(device_position, 1);
        return isosurface_shadow_transparency_texture(texture_position.xyz);
}

float isosurface_shadow_transparency_world(const vec3 world_position)
{
        const vec4 texture_position = volume_coordinates.world_to_texture_matrix * vec4(world_position, 1);
        return isosurface_shadow_transparency_texture(texture_position.xyz);
}

#endif

#endif

#endif
