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

#ifndef VOLUME_SHADOW_GLSL
#define VOLUME_SHADOW_GLSL

#include "volume_shadow_isosurface.glsl"
#include "volume_shadow_mesh.glsl"

#if defined(IMAGE)

#if !(defined(OPACITY) || defined(TRANSPARENCY))

float shadow_transparency_texture(const vec3 texture_position)
{
        return isosurface_shadow_transparency_texture(texture_position);
}

#else

float shadow_transparency(const float mesh_shadow, const float isosurface_shadow)
{
        return mesh_shadow * isosurface_shadow;
}

float shadow_transparency_texture(const vec3 texture_position)
{
        const float mesh_shadow = mesh_shadow_transparency_texture(texture_position);
        const float isosurface_shadow = isosurface_shadow_transparency_texture(texture_position);
        return shadow_transparency(mesh_shadow, isosurface_shadow);
}

#ifdef RAY_TRACING
float shadow_transparency_world(const vec3 world_position, const vec3 geometric_normal)
{
        const float mesh_shadow = mesh_shadow_transparency_world(world_position, geometric_normal);
        if (is_volume())
        {
                return mesh_shadow;
        }
        const float isosurface_shadow = isosurface_shadow_transparency_world(world_position);
        return shadow_transparency(mesh_shadow, isosurface_shadow);
}
#else
float shadow_transparency_device(const vec3 device_position)
{
        const float mesh_shadow = mesh_shadow_transparency_device(device_position);
        if (is_volume())
        {
                return mesh_shadow;
        }
        const float isosurface_shadow = isosurface_shadow_transparency_device(device_position);
        return shadow_transparency(mesh_shadow, isosurface_shadow);
}
#endif

#endif

#elif defined(OPACITY) || defined(TRANSPARENCY)

#ifdef RAY_TRACING
float shadow_transparency_world(const vec3 world_position, const vec3 geometric_normal)
{
        return mesh_shadow_transparency_world(world_position, geometric_normal);
}
#else
float shadow_transparency_device(const vec3 device_position)
{
        return mesh_shadow_transparency_device(device_position);
}
#endif

#endif

#endif
