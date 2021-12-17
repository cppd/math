/*
Copyright (C) 2017-2021 Topological Manifold

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

/*
Mike Bailey, Steve Cunningham.
Graphics Shaders. Theory and Practice. Second Edition.
CRC Press, 2012.
15. Using Shaders for Scientific Visualization

Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.
5. Shading Basics
14. Volumetric and Translucency Rendering

Klaus Engel, Markus Hadwiger, Joe M. Kniss,
Christof Rezk-Salama, Daniel Weiskopf.
Real-Time Volume Graphics.
A K Peters, Ltd, 2006.
*/

#extension GL_GOOGLE_include_directive : enable
#include "shading_ggx_diffuse.glsl"
#include "volume_fragments.glsl"
#include "volume_image.glsl"
#include "volume_intersect.glsl"
#include "volume_out.glsl"

layout(set = 0, binding = 0, std140) uniform Drawing
{
        vec3 lighting_color;
        vec3 background_color;
        vec3 wireframe_color;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float normal_length;
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

#define COLOR_ADD(c)                \
        do                          \
        {                           \
                if (color_add((c))) \
                {                   \
                        return;     \
                }                   \
        } while (false)

#if defined(FRAGMENTS)

void draw_fragments()
{
        for (; !fragments_empty(); fragments_pop())
        {
                COLOR_ADD(fragment_color(fragments_top()));
        }
}

#endif

#if defined(IMAGE)

void draw_image_as_volume(vec3 image_dir, const vec3 image_org, float depth_dir, const float depth_org)
{
        const float length_in_samples = length(image_size() * image_dir);
        const float sample_end = length_in_samples;

        image_dir /= length_in_samples;
        depth_dir /= length_in_samples;

        float s = 0.5;

        if (!fragments_empty() && s < sample_end)
        {
                Fragment fragment = fragments_top();
                float volume_depth = depth_org + s * depth_dir;

                while (true)
                {
                        while (volume_depth <= fragment.depth)
                        {
                                vec3 p = image_org + s * image_dir;
                                COLOR_ADD(volume_color(p, drawing.lighting_color));
                                if (++s >= sample_end)
                                {
                                        break;
                                }
                                volume_depth = depth_org + s * depth_dir;
                        }

                        if (s >= sample_end)
                        {
                                break;
                        }

                        while (fragment.depth <= volume_depth)
                        {
                                COLOR_ADD(fragment_color(fragment));
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

        for (; s < sample_end; ++s)
        {
                vec3 p = image_org + s * image_dir;
                COLOR_ADD(volume_color(p, drawing.lighting_color));
        }

        for (; !fragments_empty(); fragments_pop())
        {
                COLOR_ADD(fragment_color(fragments_top()));
        }
}

void draw_image_as_isosurface(vec3 image_dir, const vec3 image_org, float depth_dir, const float depth_org)
{
        const float length_in_samples = ceil(length(image_size() * image_dir));
        const float sample_end = length_in_samples + 1;

        image_dir /= length_in_samples;
        depth_dir /= length_in_samples;

        float s = 1;

        if (!fragments_empty() && s < sample_end)
        {
                Fragment fragment = fragments_top();

                while (fragment.depth <= depth_org)
                {
                        COLOR_ADD(fragment_color(fragment));
                        if (fragments_pop(), fragments_empty())
                        {
                                break;
                        }
                        fragment = fragments_top();
                }
        }

        float prev_sign = s < sample_end ? isosurface_sign(image_org) : 0;

        if (!fragments_empty() && s < sample_end)
        {
                Fragment fragment = fragments_top();

                float volume_depth = depth_org + s * depth_dir;

                while (true)
                {
                        while (volume_depth <= fragment.depth)
                        {
                                vec3 p = image_org + s * image_dir;
                                float next_sign = isosurface_sign(p);
                                if (next_sign != prev_sign)
                                {
                                        vec3 prev_p = image_org + (s - 1) * image_dir;
                                        p = find_isosurface(prev_p, p, prev_sign);
                                        prev_sign = next_sign;

                                        COLOR_ADD(isosurface_color(
                                                p, drawing.direction_to_camera, drawing.direction_to_light,
                                                drawing.lighting_color));
                                }

                                if (++s >= sample_end)
                                {
                                        break;
                                }

                                volume_depth = depth_org + s * depth_dir;
                        }

                        if (s >= sample_end)
                        {
                                break;
                        }

                        vec3 p = image_org + s * image_dir;
                        float next_sign = isosurface_sign(p);
                        if (next_sign != prev_sign)
                        {
                                vec4 prev_v = vec4(image_org, depth_org) + (s - 1) * vec4(image_dir, depth_dir);
                                vec4 v = find_isosurface(prev_v, vec4(p, volume_depth), prev_sign);
                                prev_sign = next_sign;

                                while (fragment.depth <= v.w)
                                {
                                        COLOR_ADD(fragment_color(fragment));
                                        if (fragments_pop(), fragments_empty())
                                        {
                                                break;
                                        }
                                        fragment = fragments_top();
                                };

                                COLOR_ADD(isosurface_color(
                                        v.xyz, drawing.direction_to_camera, drawing.direction_to_light,
                                        drawing.lighting_color));
                        }

                        if (!fragments_empty())
                        {
                                while (fragment.depth <= volume_depth)
                                {
                                        COLOR_ADD(fragment_color(fragment));
                                        if (fragments_pop(), fragments_empty())
                                        {
                                                break;
                                        }
                                        fragment = fragments_top();
                                }
                        }

                        if (++s >= sample_end)
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
                COLOR_ADD(fragment_color(fragments_top()));
        }

        for (; s < sample_end; ++s)
        {
                vec3 p = image_org + s * image_dir;
                float next_sign = isosurface_sign(p);
                if (next_sign == prev_sign)
                {
                        continue;
                }

                vec3 prev_p = image_org + (s - 1) * image_dir;
                p = find_isosurface(prev_p, p, prev_sign);
                prev_sign = next_sign;

                COLOR_ADD(isosurface_color(
                        p, drawing.direction_to_camera, drawing.direction_to_light, drawing.lighting_color));
        }
}

bool intersect(
        const vec3 ray_org,
        const vec3 ray_dir,
        const vec3 planes_min,
        const vec3 planes_max,
        out float near,
        out float far)
{
        if (!volume_intersect(ray_org, ray_dir, planes_min, planes_max, near, far))
        {
                return false;
        }
        if (drawing.clip_plane_enabled)
        {
                if (!clip_plane_intersect(ray_org, ray_dir, image_clip_plane_equation(), near, far))
                {
                        return false;
                }
        }
        return true;
}

bool intersect(const bool exact, const vec3 ray_org, const vec3 ray_dir, out float first, out float second)
{
        if (exact)
        {
                return intersect(ray_org, ray_dir, vec3(0), vec3(1), first, second);
        }
        const vec3 region = vec3(0.5) / image_size();
        return intersect(ray_org, ray_dir, -region, vec3(1) + region, first, second);
}

void draw_without_volume()
{
        color_init();
#if defined(FRAGMENTS)
        draw_fragments();
#endif
        color_set();
}

void main(void)
{
        fragments_build();

        const vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

        const vec3 ray_org = image_ray_org(device_coordinates);
        const vec3 ray_dir = image_ray_dir();

        const bool draw_as_volume = is_volume();

        float first;
        float second;
        if (!intersect(draw_as_volume, ray_org, ray_dir, first, second))
        {
                draw_without_volume();
                return;
        }

        const vec3 image_org = ray_org + ray_dir * first;
        const float depth_org = image_depth_org(image_org);
        const float depth_limit = texelFetch(depth_image, ivec2(gl_FragCoord.xy), gl_SampleID).r;
        const float depth_dir_limit = depth_limit - depth_org;
        if (depth_dir_limit <= 0)
        {
                draw_without_volume();
                return;
        }

        vec3 image_dir = ray_dir * (second - first);
        float depth_dir = image_depth_dir(image_dir);
        if (depth_dir > depth_dir_limit)
        {
                image_dir *= depth_dir_limit / depth_dir;
                depth_dir = depth_dir_limit;
        }

        color_init();
        if (draw_as_volume)
        {
                draw_image_as_volume(image_dir, image_org, depth_dir, depth_org);
        }
        else
        {
                draw_image_as_isosurface(image_dir, image_org, depth_dir, depth_org);
        }
        color_set();
}

#elif defined(FRAGMENTS)

void main()
{
        fragments_build();

        color_init();
        draw_fragments();
        color_set();
}

#else
#error IMAGE or FRAGMENTS not defined
#endif
