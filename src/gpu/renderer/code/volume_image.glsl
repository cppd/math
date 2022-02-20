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
#include "volume_in.glsl"

const int ISOSURFACE_ITERATION_COUNT = 5;

bool is_volume()
{
        return volume.color_volume || !volume.isosurface;
}

ivec3 image_size()
{
        return textureSize(image, 0);
}

vec4 image_clip_plane_equation()
{
        return coordinates.clip_plane_equation;
}

vec3 image_ray_org(const vec2 device_coordinates)
{
        return (coordinates.inverse_mvp_matrix * vec4(device_coordinates, 0, 1)).xyz;
}

vec3 image_ray_dir()
{
        return normalize(mat3(coordinates.inverse_mvp_matrix) * vec3(0, 0, 1));
}

float image_depth_org(const vec3 org)
{
        return dot(coordinates.third_row_of_mvp, vec4(org, 1));
}

float image_depth_dir(const vec3 dir)
{
        return dot(coordinates.third_row_of_mvp.xyz, dir);
}

float scalar_volume_value(const vec3 p)
{
        const float image_value = texture(image, p).r;
        const float value = (image_value - volume.window_offset) * volume.window_scale;
        return clamp(value, 0, 1);
}

vec4 color_volume_value(const vec3 p)
{
        if (volume.color_volume)
        {
                return texture(image, p);
        }
        const float value = scalar_volume_value(p);
        // return texture(transfer_function, value);
        return vec4(volume.color, value);
}

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

float isosurface_sign(const vec3 p)
{
        return sign(scalar_volume_value(p) - volume.isovalue);
}

vec3 find_isosurface(vec3 a, vec3 b, const float sign_a)
{
        for (int i = 0; i < ISOSURFACE_ITERATION_COUNT; ++i)
        {
                const vec3 m = 0.5 * (a + b);
                if (sign_a == sign(scalar_volume_value(m) - volume.isovalue))
                {
                        a = m;
                }
                else
                {
                        b = m;
                }
        }
        return 0.5 * (a + b);
}

vec4 find_isosurface(vec4 a, vec4 b, const float sign_a)
{
        for (int i = 0; i < ISOSURFACE_ITERATION_COUNT; ++i)
        {
                const vec4 m = 0.5 * (a + b);
                if (sign_a == sign(scalar_volume_value(m.xyz) - volume.isovalue))
                {
                        a = m;
                }
                else
                {
                        b = m;
                }
        }
        return 0.5 * (a + b);
}

#endif
