/*
Copyright (C) 2017-2020 Topological Manifold

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

// Mike Bailey, Steve Cunningham.
// Graphics Shaders. Theory and Practice. Second Edition.
// CRC Press, 2012.
// 15. Using Shaders for Scientific Visualization.

// Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
// Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
// Real-Time Rendering. Fourth Edition.
// CRC Press, 2018.
// 5. Shading Basics.
// 14. Volumetric and Translucency Rendering.

// Klaus Engel, Markus Hadwiger, Joe M. Kniss,
// Christof Rezk-Salama, Daniel Weiskopf.
// Real-Time Volume Graphics.
// A K Peters, Ltd, 2006.

#version 450

layout(set = 0, std140, binding = 0) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        vec3 background_color;
        float normal_length;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float default_ns;
        vec3 light_a;
        vec3 light_d;
        vec3 light_s;
        bool show_materials;
        bool show_wireframe;
        bool show_shadow;
        bool show_fog;
        bool show_smooth;
        vec3 clip_plane_color;
        vec4 clip_plane_equation;
        bool clip_plane_enabled;
        vec3 direction_to_light;
        vec3 direction_to_camera;
        vec2 viewport_center;
        vec2 viewport_factor;
}
drawing;

//

layout(set = 1, std140, binding = 0) uniform Coordinates
{
        mat4 inverse_mvp_matrix;
        vec4 clip_plane_equation;
        vec3 gradient_h;
        mat3 normal_matrix;
}
coordinates;

layout(set = 1, std140, binding = 1) uniform Volume
{
        float window_offset;
        float window_scale;
        float transparency;
        bool isosurface;
        float isovalue;
        bool color_volume;
}
volume;

layout(set = 1, binding = 2) uniform sampler3D image;
layout(set = 1, binding = 3) uniform sampler1D transfer_function;

//

layout(location = 0) out vec4 out_color;

//

vec4 scalar_volume_premultiplied_alphas(vec3 coordinates)
{
        float value = texture(image, coordinates).r;
        value = (value - volume.window_offset) * volume.window_scale;
        vec4 color = texture(transfer_function, clamp(value, 0, 1));
        color.a = clamp(color.a * volume.transparency, 0, 1);
        color.rgb *= color.a;
        return color;
}

vec4 color_volume_premultiplied_alphas(vec3 coordinates)
{
        vec4 color = texture(image, coordinates);
        color.a = clamp(color.a * volume.transparency, 0, 1);
        color.rgb *= color.a;
        return color;
}

bool intersect(vec3 ray_org, vec3 ray_dir, out float first, out float second)
{
        float f_max = -1e38;
        float b_min = 1e38;

        // На основе функции из ParallelotopeOrtho для случая единичного куба текстурных координат
        for (int i = 0; i < 3; ++i)
        {
                float s = ray_dir[i]; // dot(ray_dir, planes[i].n);
                if (s == 0)
                {
                        float d = ray_org[i]; // dot(ray_org, planes[i].n);
                        if (d > 1 || d < 0)
                        {
                                // параллельно плоскостям и снаружи
                                return false;
                        }
                        // внутри плоскостей
                        continue;
                }

                float d = ray_org[i]; // dot(ray_org, planes[i].n);
                float alpha1 = (1 - d) / s;
                // d и s имеют противоположный знак для другой плоскости
                float alpha2 = d / -s;

                if (s < 0)
                {
                        // пересечение снаружи для первой плоскости
                        // пересечение внутри для второй плоскости
                        f_max = max(alpha1, f_max);
                        b_min = min(alpha2, b_min);
                }
                else
                {
                        // пересечение внутри для первой плоскости
                        // пересечение снаружи для второй плоскости
                        b_min = min(alpha1, b_min);
                        f_max = max(alpha2, f_max);
                }

                if (b_min <= 0 || b_min < f_max)
                {
                        return false;
                }
        }

        // На основе функции из Parallelotope для случая одной плоскости
        if (drawing.clip_plane_enabled)
        {
                do
                {
                        vec3 n = coordinates.clip_plane_equation.xyz;
                        float d = coordinates.clip_plane_equation.w;

                        float s = dot(ray_dir, n);
                        if (s == 0)
                        {
                                if (dot(ray_org, n) > d)
                                {
                                        // параллельно плоскости и снаружи
                                        return false;
                                }
                                // внутри плоскости
                                continue;
                        }

                        float alpha = (d - dot(ray_org, n)) / s;

                        if (s < 0)
                        {
                                // пересечение снаружи
                                f_max = max(alpha, f_max);
                        }
                        else
                        {
                                // пересечение внутри
                                b_min = min(alpha, b_min);
                        }

                        if (b_min <= 0 || b_min < f_max)
                        {
                                return false;
                        }
                } while (false);
        }

        first = f_max;
        second = b_min;

        return true;
}

void main(void)
{
        vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

        vec3 ray_org = (coordinates.inverse_mvp_matrix * vec4(device_coordinates, 0, 1)).xyz;
        vec3 ray_dir = normalize(mat3(coordinates.inverse_mvp_matrix) * vec3(0, 0, 1));

        float first;
        float second;
        if (!intersect(ray_org, ray_dir, first, second))
        {
                discard;
        }

        vec3 direction = ray_dir * (second - first);
        float direction_length_in_texels = length(textureSize(image, 0) * direction);
        vec3 direction_step = direction / direction_length_in_texels;
        int samples = int(trunc(direction_length_in_texels));

        const float MIN_TRANSPARENCY = 1.0 / 256;
        float transparency = 1; // transparency = 1 - α
        vec3 color = vec3(0);
        vec3 pos = ray_org + ray_dir * first;

        if (volume.color_volume)
        {
                for (int s = 0; s < samples; ++s, pos += direction_step)
                {
                        vec4 c = color_volume_premultiplied_alphas(pos);
                        color += transparency * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                break;
                        }
                }
        }
        else
        {
                for (int s = 0; s < samples; ++s, pos += direction_step)
                {
                        vec4 c = scalar_volume_premultiplied_alphas(pos);
                        color += transparency * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                break;
                        }
                }
        }

        // srcColorBlendFactor = VK_BLEND_FACTOR_ONE
        // dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA
        out_color = vec4(color, transparency);
}
