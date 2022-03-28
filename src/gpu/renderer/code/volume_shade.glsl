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

#ifndef VOLUME_SHADE_GLSL
#define VOLUME_SHADE_GLSL

#if defined(IMAGE)

#include "ray_tracing_intersection.glsl"
#include "shade.glsl"
#include "transparency.glsl"
#include "volume_image.glsl"
#include "volume_in.glsl"
#include "volume_intersect.glsl"

#ifdef RAY_TRACING

float mesh_shadow_transparency(const vec3 world_position)
{
        const vec3 org = world_position;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 0 : 1;
}

float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 world_position = coordinates.texture_to_world_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}

float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 world_position = coordinates.device_to_world_matrix * vec4(device_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}

#else

float mesh_shadow_transparency(const vec3 shadow_position)
{
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 0 : 1;
}

float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 shadow_position = coordinates.texture_to_shadow_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(shadow_position.xyz);
}

float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 shadow_position = coordinates.device_to_shadow_matrix * vec4(device_position, 1);
        return mesh_shadow_transparency(shadow_position.xyz);
}

#endif

//

float isosurface_shadow_transparency_texture(const vec3 texture_position)
{
        const vec3 direction_to_light = normalize(coordinates.world_to_texture_matrix * drawing.direction_to_light);
        return isosurface_intersect(texture_position, direction_to_light) ? 0 : 1;
}

float isosurface_shadow_transparency_device(const vec3 device_position)
{
        const vec4 texture_position = coordinates.device_to_texture_matrix * vec4(device_position, 1);
        return isosurface_shadow_transparency_texture(texture_position.xyz);
}

//

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

float shadow_transparency_device(const vec3 device_position)
{
        const float mesh_shadow = mesh_shadow_transparency_device(device_position);
        const float isosurface_shadow = isosurface_shadow_transparency_device(device_position);
        return shadow_transparency(mesh_shadow, isosurface_shadow);
}

//

vec4 isosurface_color(const vec3 texture_position)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        const vec3 world_normal = normalize(coordinates.gradient_to_world_matrix * volume_gradient(texture_position));
        const vec3 n = faceforward(world_normal, -v, world_normal);

        vec3 color;

        if (drawing.show_shadow)
        {
                const float transparency = shadow_transparency_texture(texture_position);

                const Lighting lighting = shade(
                        volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                        ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, volume.ambient, transparency);

                color = lighting.front + lighting.side;
        }
        else
        {
                color =
                        shade(volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, volume.ambient);
        }

        return vec4(color, volume.isosurface_alpha);
}

#endif

vec4 fragment_color(const Fragment fragment)
{
        const FragmentData d = fragment_data(fragment);
        return d.color;
}

#endif
