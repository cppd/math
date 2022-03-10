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
float mesh_shadow_transparency(const vec3 p)
{
        const vec3 org = (coordinates.texture_to_world_matrix * vec4(p, 1)).xyz;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 0 : 1;
}
#else
float mesh_shadow_transparency(const vec3 p)
{
        const vec3 shadow_position = (shadow_matrix.texture_to_shadow * vec4(p, 1)).xyz;
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 0 : 1;
}
#endif

float isosurface_shadow_transparency(const vec3 p)
{
        const vec3 direction_to_light = normalize(coordinates.world_to_texture_matrix * drawing.direction_to_light);
        return isosurface_intersect(p, direction_to_light) ? 0 : 1;
}

float shadow_transparency(const vec3 p)
{
        return mesh_shadow_transparency(p) * isosurface_shadow_transparency(p);
}

vec4 isosurface_color(const vec3 p)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        const vec3 world_normal = normalize(coordinates.gradient_to_world_matrix * volume_gradient(p));
        const vec3 n = faceforward(world_normal, -v, world_normal);

        const vec3 shade_color = drawing.show_shadow
                                         ? shade(volume.color, volume.metalness, volume.roughness, n, v, l,
                                                 ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                                                 drawing.lighting_color, volume.ambient, shadow_transparency(p))
                                         : shade(volume.color, volume.metalness, volume.roughness, n, v, l,
                                                 ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                                                 drawing.lighting_color, volume.ambient);

        return vec4(shade_color, volume.isosurface_alpha);
}

#endif

vec4 fragment_color(const Fragment fragment)
{
        const FragmentData d = fragment_data(fragment);
        return d.color;
}

#endif
