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

#ifndef VOLUME_INTERSECT
#define VOLUME_INTERSECT

#include "convex_intersection.glsl"
#include "volume_image.glsl"
#include "volume_in.glsl"

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
                if (!plane_intersection(org, dir, coordinates.clip_plane_equation, near, far))
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

#endif

#endif
