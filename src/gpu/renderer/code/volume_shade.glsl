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

#include "ray_tracing_intersection.glsl"
#include "shade.glsl"
#include "transparency.glsl"
#include "volume_in.glsl"

#if defined(IMAGE)
#include "volume_image.glsl"
#include "volume_intersect.glsl"
#endif

//

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

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 world_position = volume_coordinates.texture_to_world_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}
#endif

#if defined(FRAGMENTS)
float mesh_shadow_transparency_device(const vec3 device_position)
{
        const vec4 world_position = coordinates.device_to_world * vec4(device_position, 1);
        return mesh_shadow_transparency(world_position.xyz);
}
#endif

#else

float mesh_shadow_transparency(const vec3 shadow_position)
{
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 0 : 1;
}

#if defined(IMAGE)
float mesh_shadow_transparency_texture(const vec3 texture_position)
{
        const vec4 shadow_position = volume_coordinates.texture_to_shadow_matrix * vec4(texture_position, 1);
        return mesh_shadow_transparency(shadow_position.xyz);
}
#endif

#if defined(FRAGMENTS)
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

#if defined(FRAGMENTS)
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

#if defined(FRAGMENTS)
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

#elif defined(FRAGMENTS)

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

//

#if !defined(FRAGMENTS)

vec4 fragment_color(const Fragment)
{
        return vec4(0);
}

#else

vec4 fragment_color(const Fragment fragment)
{
        const FragmentData d = fragment_data(fragment);

        if (d.n == vec3(0))
        {
                return d.color;
        }

        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        vec3 color;

        if (drawing.show_shadow)
        {
                const vec3 device_position = vec3(device_coordinates, d.depth);
                const float transparency = shadow_transparency_device(device_position);

                const Lighting lighting =
                        shade(d.color.rgb, d.metalness, d.roughness, d.n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, d.ambient, transparency);

                color = lighting.front + lighting.side;
        }
        else
        {
                color =
                        shade(d.color.rgb, d.metalness, d.roughness, d.n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, d.ambient);
        }

        if (d.edge_factor > 0)
        {
                color = mix(color, drawing.wireframe_color, d.edge_factor);
        }

        return vec4(color, d.color.a);
}

#endif

//

#endif
