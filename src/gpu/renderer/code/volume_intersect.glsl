/*
Copyright (C) 2017-2025 Topological Manifold

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

#ifndef VOLUME_INTERSECT_GLSL
#define VOLUME_INTERSECT_GLSL

#if defined(IMAGE)

#include "convex_intersection.glsl"
#include "volume_image.glsl"
#include "volume_in.glsl"

const int ISOSURFACE_ITERATION_COUNT = 5;
const float ISOSURFACE_INTERSECTION_OFFSET = 0.1;

//

bool volume_intersect(
        const vec3 org,
        const vec3 dir,
        const vec3 box_min,
        const vec3 box_max,
        out float near,
        out float far)
{
        if (!box_intersection(org, dir, box_min, box_max, near, far))
        {
                return false;
        }

        if (drawing.clip_plane_enabled)
        {
                if (!plane_intersection(org, dir, volume_coordinates.clip_plane, near, far))
                {
                        return false;
                }
        }

        return true;
}

bool volume_intersect(const vec3 org, const vec3 dir, out float first, out float second)
{
        if (is_volume())
        {
                return volume_intersect(org, dir, vec3(0), vec3(1), first, second);
        }

        const vec3 region = vec3(0.5) / textureSize(image, 0);
        return volume_intersect(org, dir, -region, vec3(1) + region, first, second);
}

//

float isosurface_sign(const vec3 p)
{
        return sign(scalar_volume_value(p) - volume.isovalue);
}

vec3 isosurface_intersect(vec3 a, vec3 b, const float sign_a)
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

vec4 isosurface_intersect(vec4 a, vec4 b, const float sign_a)
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

//

bool isosurface_intersect(const vec3 org, const vec3 dir)
{
        float first;
        float second;
        if (!volume_intersect(org, dir, first, second))
        {
                return false;
        }

        const float length_in_samples = ceil(length(textureSize(image, 0) * dir) * (second - first));
        const float sample_length = (second - first) / length_in_samples;

        const vec3 image_org = org + dir * first;
        const vec3 image_dir = dir * sample_length;

        const float prev_sign = isosurface_sign(image_org + ISOSURFACE_INTERSECTION_OFFSET * image_dir);

        for (float s = 1; s <= length_in_samples; ++s)
        {
                const vec3 next_p = image_org + s * image_dir;
                const float next_sign = isosurface_sign(next_p);
                if (next_sign != prev_sign)
                {
                        return true;
                }
        }

        return false;
}

#endif

#endif
