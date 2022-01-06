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

#include "shading_ggx_diffuse.glsl"
#include "shading_metalness.glsl"
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
        float value = texture(image, p).r;
        value = (value - volume.window_offset) * volume.window_scale;
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

vec3 world_normal(const vec3 p)
{
        return normalize(coordinates.normal_matrix * gradient(p));
}

vec3 shade(const vec3 p, const vec3 v, const vec3 l)
{
        const vec3 wn = world_normal(p);
        const vec3 n = faceforward(wn, -v, wn);

        const vec3 f0 = shading_compute_metalness_f0(volume.color, volume.metalness);
        const vec3 rho_ss = shading_compute_metalness_rho_ss(volume.color, volume.metalness);

        return shading_ggx_diffuse(
                volume.roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness,
                ggx_f1_albedo_cosine_weighted_average);
}

vec4 volume_color(const vec3 p, const vec3 lighting_color)
{
        vec4 color = color_volume_value(p);
        color.rgb *= lighting_color * volume.ambient;
        color.a = clamp(color.a * volume.volume_alpha_coefficient, 0, 1);
        return color;
}

float isosurface_sign(const vec3 p)
{
        return sign(scalar_volume_value(p) - volume.isovalue);
}

vec4 isosurface_color(const vec3 p, const vec3 v, const vec3 l, const vec3 lighting_color)
{
        vec3 color = volume.ambient * volume.color + lighting_color * shade(p, v, l);
        return vec4(color, volume.isosurface_alpha);
}

vec3 find_isosurface(vec3 a, vec3 b, const float sign_a)
{
        for (int i = 0; i < ISOSURFACE_ITERATION_COUNT; ++i)
        {
                vec3 m = 0.5 * (a + b);
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
                vec4 m = 0.5 * (a + b);
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
