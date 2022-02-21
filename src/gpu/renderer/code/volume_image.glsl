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

#ifndef VOLUME_IMAGE
#define VOLUME_IMAGE

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

#endif

#endif
