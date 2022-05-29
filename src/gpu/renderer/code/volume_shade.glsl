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

#include "fragments.glsl"
#include "shade.glsl"
#include "visibility.glsl"
#include "volume_in.glsl"

#if defined(IMAGE)
#include "volume_image.glsl"
#include "volume_intersect.glsl"
#endif

//

#ifdef RAY_TRACING

float mesh_shadow_transparency(const vec3 world_position)
{
        if (!drawing.clip_plane_enabled)
        {
                return shadow_transparency(world_position, drawing.direction_to_light, acceleration_structure);
        }
        return shadow_transparency(
                world_position, drawing.direction_to_light, drawing.clip_plane_equation, acceleration_structure);
}

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 world_position = volume_coordinates.texture_to_world_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}
#endif

#if defined(OPACITY) || defined(TRANSPARENCY)
float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 world_position = coordinates.device_to_world * vec4(device_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}
#endif

#else

float mesh_shadow_transparency(const vec3 shadow_position)
{
        return shadow_transparency(shadow_position, shadow_mapping_texture);
}

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 shadow_position = volume_coordinates.texture_to_shadow_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(shadow_position.xyz);
}
#endif

#if defined(OPACITY) || defined(TRANSPARENCY)
float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 shadow_position = coordinates.device_to_shadow * vec4(device_position, 1);
        return mesh_shadow_transparency(shadow_position.xyz);
}
#endif

#endif

//

#if defined(IMAGE)

float isosurface_shadow_transparency_texture(const vec3 texture_position)
{
        const vec3 direction_to_light =
                normalize(volume_coordinates.world_to_texture_matrix * drawing.direction_to_light);
        return isosurface_intersect(texture_position, direction_to_light) ? 0 : 1;
}

#if defined(OPACITY) || defined(TRANSPARENCY)
float isosurface_shadow_transparency_device(const vec3 device_position)
{
        const vec4 texture_position = volume_coordinates.device_to_texture_matrix * vec4(device_position, 1);
        return isosurface_shadow_transparency_texture(texture_position.xyz);
}
#endif

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

#if defined(OPACITY) || defined(TRANSPARENCY)
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

#elif defined(OPACITY) || defined(TRANSPARENCY)

float shadow_transparency_device(const vec3 device_position)
{
        return mesh_shadow_transparency_device(device_position);
}

#endif

//

#if defined(IMAGE)

vec4 isosurface_color(const vec3 texture_position)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        const vec3 world_normal =
                normalize(volume_coordinates.gradient_to_world_matrix * volume_gradient(texture_position));
        const vec3 n = faceforward(world_normal, -v, world_normal);

        vec3 color = volume.ambient * volume.color;

        if (drawing.show_shadow)
        {
                const float shadow_transparency = dot(n, l) > 0 ? shadow_transparency_texture(texture_position) : 0;

                color +=
                        shade(volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, shadow_transparency);
        }
        else
        {
                color +=
                        shade(volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color);
        }

        return vec4(color, volume.isosurface_alpha);
}

#endif

//

#if !(defined(OPACITY) || defined(TRANSPARENCY))

vec4 fragment_color(const TransparencyFragment fragment)
{
        return vec4(0);
}

vec4 fragment_color(const OpacityFragment fragment)
{
        return vec4(0);
}

#else

vec4 fragment_color(const Fragment fragment)
{
        if (fragment.n == vec3(0))
        {
                return fragment.color;
        }

        const vec3 n = fragment.n;
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        vec3 color = fragment.color.rgb * fragment.ambient;

        if (drawing.show_shadow)
        {
                const float shadow_transparency =
                        dot(n, l) > 0 ? shadow_transparency_device(vec3(device_coordinates, fragment.depth)) : 0;

                color +=
                        shade(fragment.color.rgb, fragment.metalness, fragment.roughness, n, v, l,
                              ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                              drawing.lighting_color, shadow_transparency);
        }
        else
        {
                color += shade(
                        fragment.color.rgb, fragment.metalness, fragment.roughness, n, v, l,
                        ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color);
        }

        if (fragment.edge_factor > 0)
        {
                color = mix(color, drawing.wireframe_color, fragment.edge_factor);
        }

        return vec4(color, fragment.color.a);
}

vec4 fragment_color(const TransparencyFragment fragment)
{
        return fragment_color(to_fragment(fragment));
}

vec4 fragment_color(const OpacityFragment fragment)
{
        return fragment_color(to_fragment(fragment));
}

#endif

//

#endif
