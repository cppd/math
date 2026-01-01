/*
Copyright (C) 2017-2026 Topological Manifold

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

#ifndef VOLUME_IMAGE_GLSL
#define VOLUME_IMAGE_GLSL

#if defined(IMAGE)

#include "volume_in.glsl"

bool is_volume()
{
        return volume.color_volume || !volume.isosurface;
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

vec4 volume_color(const vec3 p)
{
        vec4 color = color_volume_value(p);
        color.rgb *= drawing.lighting_color * volume.ambient;
        color.a = clamp(color.a * volume.volume_alpha_coefficient, 0, 1);
        return color;
}

vec3 volume_gradient(const vec3 p)
{
        vec3 s1;
        vec3 s2;

        s1.x = scalar_volume_value(vec3(p.x - volume_coordinates.gradient_h.x, p.y, p.z));
        s2.x = scalar_volume_value(vec3(p.x + volume_coordinates.gradient_h.x, p.y, p.z));

        s1.y = scalar_volume_value(vec3(p.x, p.y - volume_coordinates.gradient_h.y, p.z));
        s2.y = scalar_volume_value(vec3(p.x, p.y + volume_coordinates.gradient_h.y, p.z));

        s1.z = scalar_volume_value(vec3(p.x, p.y, p.z - volume_coordinates.gradient_h.z));
        s2.z = scalar_volume_value(vec3(p.x, p.y, p.z + volume_coordinates.gradient_h.z));

        return s2 - s1;
}

#endif

#endif
