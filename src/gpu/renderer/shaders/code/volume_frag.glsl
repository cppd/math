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

const float MIN_TRANSPARENCY = 1.0 / 256;
const int ISOSURFACE_ITERATION_COUNT = 5;

layout(set = 0, binding = 0, std140) uniform Drawing
{
        vec3 default_color;
        vec3 default_specular_color;
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
        uint transparency_max_node_count;
}
drawing;

layout(set = 0, binding = 1) uniform sampler2DMS depth_image;

layout(set = 0, binding = 2, r32ui) uniform restrict readonly uimage2DMS transparency_heads;
struct TransparencyNode
{
        uint color_rg;
        uint color_ba;
        float depth;
        uint next;
};
layout(set = 0, binding = 3, std430) restrict readonly buffer TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

#if defined(IMAGE)

layout(set = 1, binding = 0, std140) uniform Coordinates
{
        mat4 inverse_mvp_matrix;
        vec4 third_row_of_mvp;
        vec4 clip_plane_equation;
        vec3 gradient_h;
        mat3 normal_matrix;
}
coordinates;

layout(set = 1, binding = 1, std140) uniform Volume
{
        float window_offset;
        float window_scale;
        float volume_alpha_coefficient;
        float isosurface_alpha;
        bool isosurface;
        float isovalue;
        vec3 color;
        bool color_volume;
}
volume;

layout(set = 1, binding = 2) uniform sampler3D image;
layout(set = 1, binding = 3) uniform sampler1D transfer_function;

#endif

//

// srcColorBlendFactor = VK_BLEND_FACTOR_ONE
// dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA
layout(location = 0) out vec4 out_color;

//

const uint TRANSPARENCY_NULL_POINTER = 0xffffffff;
const uint TRANSPARENCY_MAX_NODES = 32;
struct TransparencyArrayNode
{
        uint color_rg;
        uint color_ba;
        float depth;
};
TransparencyArrayNode transparency_fragments[TRANSPARENCY_MAX_NODES];
int transparency_fragment_count;

void transparency_read_and_sort_fragments()
{
        transparency_fragment_count = 0;

#if defined(MESH)

        uint pointer = imageLoad(transparency_heads, ivec2(gl_FragCoord.xy), gl_SampleID).r;

        if (pointer == TRANSPARENCY_NULL_POINTER)
        {
                return;
        }

        while (pointer != TRANSPARENCY_NULL_POINTER && transparency_fragment_count < TRANSPARENCY_MAX_NODES)
        {
                transparency_fragments[transparency_fragment_count].color_rg = transparency_nodes[pointer].color_rg;
                transparency_fragments[transparency_fragment_count].color_ba = transparency_nodes[pointer].color_ba;
                transparency_fragments[transparency_fragment_count].depth = transparency_nodes[pointer].depth;
                pointer = transparency_nodes[pointer].next;
                ++transparency_fragment_count;
        }

        // Insertion sort
        for (int i = 1; i < transparency_fragment_count; ++i)
        {
                TransparencyArrayNode n = transparency_fragments[i];
                int j = i - 1;
                while (j >= 0 && n.depth < transparency_fragments[j].depth)
                {
                        transparency_fragments[j + 1] = transparency_fragments[j];
                        --j;
                }
                transparency_fragments[j + 1] = n;
        }

#endif
}

vec4 transparency_compute()
{
#if !defined(MESH)
        discard;
        return vec4(0);
#else
        float transparency = 1; // transparency = 1 - α
        vec3 color = vec3(0);

        for (int i = 0; i < transparency_fragment_count; ++i)
        {
                TransparencyArrayNode node = transparency_fragments[i];
                vec4 c = vec4(unpackUnorm2x16(node.color_rg), unpackUnorm2x16(node.color_ba));
                color += (transparency * c.a) * c.rgb;
                transparency *= 1.0 - c.a;
                if (transparency < MIN_TRANSPARENCY)
                {
                        break;
                }
        }
        return vec4(color, transparency);
#endif
}

#if defined(IMAGE)

float scalar_volume_value(vec3 p)
{
        float value = texture(image, p).r;
        value = (value - volume.window_offset) * volume.window_scale;
        return clamp(value, 0, 1);
}

vec4 volume_color(vec3 p)
{
        if (volume.color_volume)
        {
                vec4 color = texture(image, p);
                color.a = clamp(color.a * volume.volume_alpha_coefficient, 0, 1);
                return color;
        }
        float value = scalar_volume_value(p);
        // vec4 color = texture(transfer_function, value);
        vec4 color = vec4(volume.color * (drawing.light_a + drawing.light_d), value);
        color.a = clamp(color.a * volume.volume_alpha_coefficient, 0, 1);
        return color;
}

vec3 gradient(vec3 p)
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

vec3 world_normal(vec3 p)
{
        return normalize(coordinates.normal_matrix * gradient(p));
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

vec3 find_isosurface(vec3 a, vec3 b, float sign_a)
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

vec4 find_isosurface(vec4 a, vec4 b, float sign_a)
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

vec3 shade(vec3 p)
{
        vec3 color = volume.color * drawing.light_a;

        vec3 N = world_normal(p);
        N = faceforward(N, -drawing.direction_to_camera, N);
        vec3 L = drawing.direction_to_light;
        vec3 V = drawing.direction_to_camera;

        float diffuse = max(0, dot(N, L));
        if (diffuse > 0)
        {
                float specular = pow(max(0, dot(V, reflect(-L, N))), drawing.default_ns);
                color += diffuse * volume.color * drawing.light_d;
                color += specular * drawing.default_specular_color * drawing.light_s;
        }

        return color;
}

void main(void)
{
        transparency_read_and_sort_fragments();

        vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

        vec3 ray_org = (coordinates.inverse_mvp_matrix * vec4(device_coordinates, 0, 1)).xyz;
        vec3 ray_dir = normalize(mat3(coordinates.inverse_mvp_matrix) * vec3(0, 0, 1));

        float first;
        float second;
        if (!intersect(ray_org, ray_dir, first, second))
        {
                out_color = transparency_compute();
                return;
        }
        first = max(0, first);

        vec3 image_direction = ray_dir * (second - first);
        vec3 image_pos = ray_org + ray_dir * first;

        float depth_pos = dot(coordinates.third_row_of_mvp, vec4(image_pos, 1));
        float depth_limit = texelFetch(depth_image, ivec2(gl_FragCoord.xy), gl_SampleID).r;
        float depth_direction_limit = depth_limit - depth_pos;
        if (depth_direction_limit <= 0)
        {
                out_color = transparency_compute();
                return;
        }
        float depth_direction = dot(coordinates.third_row_of_mvp.xyz, image_direction);

        float transparency = 1; // transparency = 1 - α
        vec3 color = vec3(0);

        if (volume.color_volume || !volume.isosurface)
        {
                float length_in_samples = length(textureSize(image, 0) * image_direction);
                int sample_count =
                        int(trunc((min(depth_direction_limit, depth_direction) / depth_direction * length_in_samples)));
                float length_in_samples_r = 1.0 / length_in_samples;

                int f = 0;
                int s = 0;

                if (f < transparency_fragment_count && s < sample_count)
                {
                        TransparencyArrayNode node = transparency_fragments[f];
                        float transparency_depth = node.depth;

                        float k = 0;
                        float volume_depth = depth_pos;

                        do
                        {
                                if (volume_depth <= transparency_depth)
                                {
                                        do
                                        {
                                                vec3 p = image_pos + k * image_direction;
                                                vec4 c = volume_color(p);

                                                color += (transparency * c.a) * c.rgb;
                                                transparency *= 1.0 - c.a;
                                                if (transparency < MIN_TRANSPARENCY)
                                                {
                                                        out_color = vec4(color, transparency);
                                                        return;
                                                }

                                                if (++s >= sample_count)
                                                {
                                                        break;
                                                }

                                                k = s * length_in_samples_r;

                                                volume_depth = depth_pos + k * depth_direction;
                                        } while (volume_depth <= transparency_depth);
                                }
                                else
                                {
                                        do
                                        {
                                                vec2 rg = unpackUnorm2x16(node.color_rg);
                                                vec2 ba = unpackUnorm2x16(node.color_ba);
                                                vec4 c = vec4(rg, ba);

                                                color += (transparency * c.a) * c.rgb;
                                                transparency *= 1.0 - c.a;
                                                if (transparency < MIN_TRANSPARENCY)
                                                {
                                                        out_color = vec4(color, transparency);
                                                        return;
                                                }

                                                if (++f >= transparency_fragment_count)
                                                {
                                                        break;
                                                }

                                                node = transparency_fragments[f];

                                                transparency_depth = node.depth;
                                        } while (transparency_depth <= volume_depth);
                                }
                        } while (f < transparency_fragment_count && s < sample_count);
                }
                for (; s < sample_count; ++s)
                {
                        float k = s * length_in_samples_r;
                        vec3 p = image_pos + k * image_direction;
                        vec4 c = volume_color(p);

                        color += (transparency * c.a) * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                out_color = vec4(color, transparency);
                                return;
                        }
                }
                for (; f < transparency_fragment_count; ++f)
                {
                        TransparencyArrayNode node = transparency_fragments[f];
                        vec4 c = vec4(unpackUnorm2x16(node.color_rg), unpackUnorm2x16(node.color_ba));

                        color += (transparency * c.a) * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                out_color = vec4(color, transparency);
                                return;
                        }
                }
        }
        else
        {
                float length_coef = min(depth_direction_limit, depth_direction) / depth_direction;

                image_direction *= length_coef;
                depth_direction *= length_coef;

                int sample_count = int(ceil(length(textureSize(image, 0) * image_direction)));
                float length_in_samples_r = 1.0 / sample_count;

                int f = 0;
                int s = 1;

                if (f < transparency_fragment_count && s <= sample_count)
                {
                        TransparencyArrayNode node = transparency_fragments[f];
                        float transparency_depth = node.depth;

                        while (transparency_depth <= depth_pos)
                        {
                                vec2 rg = unpackUnorm2x16(node.color_rg);
                                vec2 ba = unpackUnorm2x16(node.color_ba);
                                vec4 c = vec4(rg, ba);

                                color += (transparency * c.a) * c.rgb;
                                transparency *= 1.0 - c.a;
                                if (transparency < MIN_TRANSPARENCY)
                                {
                                        out_color = vec4(color, transparency);
                                        return;
                                }

                                if (++f >= transparency_fragment_count)
                                {
                                        break;
                                }

                                node = transparency_fragments[f];

                                transparency_depth = node.depth;
                        }
                }

                float prev_sign = sample_count >= 1 ? sign(scalar_volume_value(image_pos) - volume.isovalue) : 0;

                if (f < transparency_fragment_count && s <= sample_count)
                {
                        TransparencyArrayNode node = transparency_fragments[f];
                        float transparency_depth = node.depth;

                        float k = length_in_samples_r;
                        float volume_depth = depth_pos + k * depth_direction;

                        do
                        {
                                while (volume_depth <= transparency_depth)
                                {
                                        vec3 p = image_pos + k * image_direction;

                                        float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                                        if (next_sign != prev_sign)
                                        {
                                                float prev_k = (s - 1) * length_in_samples_r;
                                                vec3 prev_p = image_pos + prev_k * image_direction;
                                                p = find_isosurface(prev_p, p, prev_sign);
                                                vec4 c = vec4(shade(p), volume.isosurface_alpha);

                                                color += (transparency * c.a) * c.rgb;
                                                transparency *= 1.0 - c.a;
                                                if (transparency < MIN_TRANSPARENCY)
                                                {
                                                        out_color = vec4(color, transparency);
                                                        return;
                                                }

                                                prev_sign = next_sign;
                                        }

                                        if (++s > sample_count)
                                        {
                                                break;
                                        }

                                        k = s * length_in_samples_r;

                                        volume_depth = depth_pos + k * depth_direction;
                                }

                                if (s <= sample_count)
                                {
                                        vec3 p = image_pos + k * image_direction;
                                        float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                                        if (next_sign != prev_sign)
                                        {
                                                float prev_k = (s - 1) * length_in_samples_r;
                                                vec3 prev_p = image_pos + prev_k * image_direction;
                                                float prev_depth = depth_pos + prev_k * depth_direction;
                                                vec4 prev_v = vec4(prev_p, prev_depth);
                                                vec4 v = vec4(p, volume_depth);
                                                vec4 iso_p = find_isosurface(prev_v, v, prev_sign);

                                                prev_sign = next_sign;

                                                while (transparency_depth <= iso_p.w)
                                                {
                                                        vec2 rg = unpackUnorm2x16(node.color_rg);
                                                        vec2 ba = unpackUnorm2x16(node.color_ba);
                                                        vec4 c = vec4(rg, ba);

                                                        color += (transparency * c.a) * c.rgb;
                                                        transparency *= 1.0 - c.a;
                                                        if (transparency < MIN_TRANSPARENCY)
                                                        {
                                                                out_color = vec4(color, transparency);
                                                                return;
                                                        }

                                                        if (++f >= transparency_fragment_count)
                                                        {
                                                                break;
                                                        }

                                                        node = transparency_fragments[f];

                                                        transparency_depth = node.depth;
                                                };

                                                vec4 c = vec4(shade(iso_p.xyz), volume.isosurface_alpha);

                                                color += (transparency * c.a) * c.rgb;
                                                transparency *= 1.0 - c.a;
                                                if (transparency < MIN_TRANSPARENCY)
                                                {
                                                        out_color = vec4(color, transparency);
                                                        return;
                                                }
                                        }

                                        if (f < transparency_fragment_count)
                                        {
                                                while (transparency_depth <= volume_depth)
                                                {
                                                        vec2 rg = unpackUnorm2x16(node.color_rg);
                                                        vec2 ba = unpackUnorm2x16(node.color_ba);
                                                        vec4 c = vec4(rg, ba);

                                                        color += (transparency * c.a) * c.rgb;
                                                        transparency *= 1.0 - c.a;
                                                        if (transparency < MIN_TRANSPARENCY)
                                                        {
                                                                out_color = vec4(color, transparency);
                                                                return;
                                                        }

                                                        if (++f >= transparency_fragment_count)
                                                        {
                                                                break;
                                                        }

                                                        node = transparency_fragments[f];

                                                        transparency_depth = node.depth;
                                                }
                                        }
                                }
                        } while (f < transparency_fragment_count && s <= sample_count);
                }
                for (; f < transparency_fragment_count; ++f)
                {
                        TransparencyArrayNode node = transparency_fragments[f];
                        vec4 c = vec4(unpackUnorm2x16(node.color_rg), unpackUnorm2x16(node.color_ba));

                        color += (transparency * c.a) * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                out_color = vec4(color, transparency);
                                return;
                        }
                }
                for (; s <= sample_count; ++s)
                {
                        float k = s * length_in_samples_r;
                        vec3 p = image_pos + k * image_direction;

                        float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                        if (next_sign == prev_sign)
                        {
                                continue;
                        }

                        float prev_k = (s - 1) * length_in_samples_r;
                        vec3 prev_p = image_pos + prev_k * image_direction;
                        p = find_isosurface(prev_p, p, prev_sign);
                        vec4 c = vec4(shade(p), volume.isosurface_alpha);

                        color += (transparency * c.a) * c.rgb;
                        transparency *= 1.0 - c.a;
                        if (transparency < MIN_TRANSPARENCY)
                        {
                                out_color = vec4(color, transparency);
                                return;
                        }

                        prev_sign = next_sign;
                }
        }

        out_color = vec4(color, transparency);
}

#elif defined(MESH)

void main()
{
        transparency_read_and_sort_fragments();
        out_color = transparency_compute();
}

#else
#error IMAGE or MESH not defined
#endif
