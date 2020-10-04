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
        vec3 specular_color;
        vec3 wireframe_color;
        vec3 background_color;
        float normal_length;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float lighting_intensity;
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
        float ambient;
        float diffuse;
        float specular;
        float specular_power;
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

struct Fragment
{
        uint color_rg;
        uint color_ba;
        float depth;
};

// transparency = 1 - α
float g_transparency;
vec3 g_color;
void color_init()
{
        g_transparency = 1;
        g_color = vec3(0);
}
void color_set()
{
        out_color = vec4(g_color, g_transparency);
}
bool color_add(vec4 c)
{
        g_color += (g_transparency * c.a) * c.rgb;
        g_transparency *= 1.0 - c.a;
        return g_transparency < MIN_TRANSPARENCY;
}
bool color_add(Fragment fragment)
{
        vec2 rg = unpackUnorm2x16(fragment.color_rg);
        vec2 ba = unpackUnorm2x16(fragment.color_ba);
        vec4 c = vec4(rg, ba);
        g_color += (g_transparency * c.a) * c.rgb;
        g_transparency *= 1.0 - c.a;
        return g_transparency < MIN_TRANSPARENCY;
}
#define COLOR_ADD(c)                \
        do                          \
        {                           \
                if (color_add((c))) \
                {                   \
                        return;     \
                }                   \
        } while (false)

#if !defined(MESH)

void fragments_build()
{
}
bool fragments_empty()
{
        return true;
}
void fragments_pop()
{
}
Fragment fragments_top()
{
        Fragment f;
        return f;
}

#else

const uint TRANSPARENCY_NULL_POINTER = 0xffffffff;
const uint TRANSPARENCY_MAX_NODES = 32;

Fragment g_fragments[TRANSPARENCY_MAX_NODES];
int g_fragments_count = 0;
int g_fragments_top = 0;

void fragments_sort()
{
        // Insertion sort
        for (int i = 1; i < g_fragments_count; ++i)
        {
                Fragment n = g_fragments[i];
                int j = i - 1;
                while (j >= 0 && n.depth < g_fragments[j].depth)
                {
                        g_fragments[j + 1] = g_fragments[j];
                        --j;
                }
                g_fragments[j + 1] = n;
        }
}

void fragments_build()
{
        uint pointer = imageLoad(transparency_heads, ivec2(gl_FragCoord.xy), gl_SampleID).r;

        if (pointer == TRANSPARENCY_NULL_POINTER)
        {
                return;
        }

        while (pointer != TRANSPARENCY_NULL_POINTER && g_fragments_count < TRANSPARENCY_MAX_NODES)
        {
                g_fragments[g_fragments_count].color_rg = transparency_nodes[pointer].color_rg;
                g_fragments[g_fragments_count].color_ba = transparency_nodes[pointer].color_ba;
                g_fragments[g_fragments_count].depth = transparency_nodes[pointer].depth;
                pointer = transparency_nodes[pointer].next;
                ++g_fragments_count;
        }

        fragments_sort();
}

bool fragments_empty()
{
        return g_fragments_top >= g_fragments_count;
}

void fragments_pop()
{
        ++g_fragments_top;
}

Fragment fragments_top()
{
        return g_fragments[g_fragments_top];
}

#endif

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
        vec4 color = vec4(volume.color * (volume.ambient + volume.diffuse * drawing.lighting_intensity), value);
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
        vec3 color = volume.color * volume.ambient;

        vec3 N = world_normal(p);
        N = faceforward(N, -drawing.direction_to_camera, N);
        vec3 L = drawing.direction_to_light;
        vec3 V = drawing.direction_to_camera;

        float diffuse = max(0, dot(N, L));
        if (diffuse > 0)
        {
                float specular = pow(max(0, dot(V, reflect(-L, N))), volume.specular_power);
                color += (diffuse * volume.diffuse * drawing.lighting_intensity) * volume.color;
                color += (specular * volume.specular * drawing.lighting_intensity) * drawing.specular_color;
        }

        return color;
}

vec4 isosurface_color(vec3 p)
{
        return vec4(shade(p), volume.isosurface_alpha);
}

void draw_image_as_volume(vec3 image_dir, vec3 image_org, float depth_dir, float depth_org)
{
        const float length_in_samples = length(textureSize(image, 0) * image_dir);
        const float sample_count = trunc(length_in_samples);

        image_dir /= length_in_samples;
        depth_dir /= length_in_samples;

        float s = 0.5;

        if (!fragments_empty() && s < sample_count)
        {
                Fragment fragment = fragments_top();
                float volume_depth = depth_org + s * depth_dir;

                while (true)
                {
                        while (volume_depth <= fragment.depth)
                        {
                                vec3 p = image_org + s * image_dir;
                                COLOR_ADD(volume_color(p));
                                if (++s >= sample_count)
                                {
                                        break;
                                }
                                volume_depth = depth_org + s * depth_dir;
                        }

                        if (s >= sample_count)
                        {
                                break;
                        }

                        while (fragment.depth <= volume_depth)
                        {
                                COLOR_ADD(fragment);
                                if (fragments_pop(), fragments_empty())
                                {
                                        break;
                                }
                                fragment = fragments_top();
                        }

                        if (fragments_empty())
                        {
                                break;
                        }
                }
        }

        for (; s < sample_count; ++s)
        {
                vec3 p = image_org + s * image_dir;
                COLOR_ADD(volume_color(p));
        }

        for (; !fragments_empty(); fragments_pop())
        {
                COLOR_ADD(fragments_top());
        }
}

void draw_image_as_isosurface(vec3 image_dir, vec3 image_org, float depth_dir, float depth_org)
{
        const float length_in_samples = ceil(length(textureSize(image, 0) * image_dir));
        const float length_in_samples_r = 1.0 / length_in_samples;
        const float sample_count = 1 + length_in_samples;

        image_dir /= length_in_samples;
        depth_dir /= length_in_samples;

        float s = 1;

        if (!fragments_empty() && s < sample_count)
        {
                Fragment fragment = fragments_top();

                while (fragment.depth <= depth_org)
                {
                        COLOR_ADD(fragment);
                        if (fragments_pop(), fragments_empty())
                        {
                                break;
                        }
                        fragment = fragments_top();
                }
        }

        float prev_sign = sample_count > 1 ? sign(scalar_volume_value(image_org) - volume.isovalue) : 0;

        if (!fragments_empty() && s < sample_count)
        {
                Fragment fragment = fragments_top();

                float volume_depth = depth_org + s * depth_dir;

                while (true)
                {
                        while (volume_depth <= fragment.depth)
                        {
                                vec3 p = image_org + s * image_dir;
                                float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                                if (next_sign != prev_sign)
                                {
                                        vec3 prev_p = image_org + (s - 1) * image_dir;
                                        p = find_isosurface(prev_p, p, prev_sign);
                                        prev_sign = next_sign;

                                        COLOR_ADD(isosurface_color(p));
                                }

                                if (++s >= sample_count)
                                {
                                        break;
                                }

                                volume_depth = depth_org + s * depth_dir;
                        }

                        if (s >= sample_count)
                        {
                                break;
                        }

                        vec3 p = image_org + s * image_dir;
                        float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                        if (next_sign != prev_sign)
                        {
                                vec4 prev_v = vec4(image_org, depth_org) + (s - 1) * vec4(image_dir, depth_dir);
                                vec4 v = find_isosurface(prev_v, vec4(p, volume_depth), prev_sign);
                                prev_sign = next_sign;

                                while (fragment.depth <= v.w)
                                {
                                        COLOR_ADD(fragment);
                                        if (fragments_pop(), fragments_empty())
                                        {
                                                break;
                                        }
                                        fragment = fragments_top();
                                };

                                COLOR_ADD(isosurface_color(v.xyz));
                        }

                        if (!fragments_empty())
                        {
                                while (fragment.depth <= volume_depth)
                                {
                                        COLOR_ADD(fragment);
                                        if (fragments_pop(), fragments_empty())
                                        {
                                                break;
                                        }
                                        fragment = fragments_top();
                                }
                        }

                        if (++s >= sample_count)
                        {
                                break;
                        }

                        if (fragments_empty())
                        {
                                break;
                        }

                        volume_depth = depth_org + s * depth_dir;
                }
        }

        for (; !fragments_empty(); fragments_pop())
        {
                COLOR_ADD(fragments_top());
        }

        for (; s < sample_count; ++s)
        {
                vec3 p = image_org + s * image_dir;
                float next_sign = sign(scalar_volume_value(p) - volume.isovalue);
                if (next_sign == prev_sign)
                {
                        continue;
                }

                vec3 prev_p = image_org + (s - 1) * image_dir;
                p = find_isosurface(prev_p, p, prev_sign);
                prev_sign = next_sign;

                COLOR_ADD(isosurface_color(p));
        }
}

#endif

#if defined(MESH)

void draw_fragments()
{
        for (; !fragments_empty(); fragments_pop())
        {
                COLOR_ADD(fragments_top());
        }
}

#endif

#if defined(IMAGE)

void main(void)
{
        fragments_build();

        vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

        vec3 ray_org = (coordinates.inverse_mvp_matrix * vec4(device_coordinates, 0, 1)).xyz;
        vec3 ray_dir = normalize(mat3(coordinates.inverse_mvp_matrix) * vec3(0, 0, 1));

        float first;
        float second;
        if (!intersect(ray_org, ray_dir, first, second))
        {
#if defined(MESH)
                color_init();
                draw_fragments();
                color_set();
                return;
#else
                discard;
#endif
        }
        first = max(0, first);

        vec3 image_dir = ray_dir * (second - first);
        vec3 image_org = ray_org + ray_dir * first;

        float depth_org = dot(coordinates.third_row_of_mvp, vec4(image_org, 1));
        float depth_limit = texelFetch(depth_image, ivec2(gl_FragCoord.xy), gl_SampleID).r;
        float depth_dir_limit = depth_limit - depth_org;
        if (depth_dir_limit <= 0)
        {
#if defined(MESH)
                color_init();
                draw_fragments();
                color_set();
                return;
#else
                discard;
#endif
        }
        float depth_dir = dot(coordinates.third_row_of_mvp.xyz, image_dir);

        if (depth_dir > depth_dir_limit)
        {
                image_dir *= depth_dir_limit / depth_dir;
                depth_dir = depth_dir_limit;
        }

        if (volume.color_volume)
        {
                color_init();
                draw_image_as_volume(image_dir, image_org, depth_dir, depth_org);
                color_set();
                return;
        }
        if (!volume.isosurface)
        {
                color_init();
                draw_image_as_volume(image_dir, image_org, depth_dir, depth_org);
                color_set();
                return;
        }
        color_init();
        draw_image_as_isosurface(image_dir, image_org, depth_dir, depth_org);
        color_set();
}

#elif defined(MESH)

void main()
{
        fragments_build();

        color_init();
        draw_fragments();
        color_set();
}

#else
#error IMAGE or MESH not defined
#endif
