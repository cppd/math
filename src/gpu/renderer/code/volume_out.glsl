/*
Copyright (C) 2017-2024 Topological Manifold

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

#ifndef VOLUME_OUT_GLSL
#define VOLUME_OUT_GLSL

const float MIN_TRANSPARENCY = 1.0 / 256;

// srcColorBlendFactor = VK_BLEND_FACTOR_ONE
// dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA
layout(location = 0) out vec4 out_color;

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

bool color_add(const vec4 c)
{
        g_color += (g_transparency * c.a) * c.rgb;
        g_transparency *= 1.0 - c.a;
        return g_transparency < MIN_TRANSPARENCY;
}

#endif
