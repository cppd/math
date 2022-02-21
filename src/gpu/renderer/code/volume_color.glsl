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

#if defined(IMAGE)

#include "ray_tracing_intersection.glsl"
#include "shade.glsl"
#include "volume_image.glsl"
#include "volume_in.glsl"

vec3 gradient(const vec3 p)
{
        vec3 s1;
        vec3 s2;

        s1.x = scalar_volume_value(vec3(p.x - coordinates.gradient_h.x, p.y, p.z));
        s2.x = scalar_volume_value(vec3(p.x + coordinates.gradient_h.x, p.y, p.z));

        s1.y = scalar_volume_value(vec3(p.x, p.y - coordinates.gradient_h.y, p.z));
        s2.y = scalar_volume_value(vec3(p.x, p.y + coordinates.gradient_h.y, p.z));

        s1.z = scalar_volume_value(vec3(p.x, p.y, p.z - coordinates.gradient_h.z));
        s2.z = scalar_volume_value(vec3(p.x, p.y, p.z + coordinates.gradient_h.z));

        return s2 - s1;
}

#ifdef RAY_TRACING
float shadow_weight(const vec3 p)
{
        const vec3 org = (coordinates.texture_to_world_matrix * vec4(p, 1)).xyz;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 1 : 0;
}
#else
float shadow_weight(const vec3 p)
{
        const vec3 shadow_position = (shadow_matrix.texture_to_shadow * vec4(p, 1)).xyz;
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 1 : 0;
}
#endif

vec4 volume_color(const vec3 p)
{
        vec4 color = color_volume_value(p);
        color.rgb *= drawing.lighting_color * volume.ambient;
        color.a = clamp(color.a * volume.volume_alpha_coefficient, 0, 1);
        return color;
}

vec4 isosurface_color(const vec3 p)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        const vec3 world_normal = normalize(coordinates.gradient_to_world_matrix * gradient(p));
        const vec3 n = faceforward(world_normal, -v, world_normal);

        const vec3 shade_color = drawing.show_shadow
                                         ? shade(volume.color, volume.metalness, volume.roughness, n, v, l,
                                                 ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                                                 drawing.lighting_color, volume.ambient, shadow_weight(p))
                                         : shade(volume.color, volume.metalness, volume.roughness, n, v, l,
                                                 ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                                                 drawing.lighting_color, volume.ambient);

        return vec4(shade_color, volume.isosurface_alpha);
}

#endif
