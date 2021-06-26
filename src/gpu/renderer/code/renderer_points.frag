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

#version 450

#define FRAGMENT_SHADER

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

vec3 fog(vec3 fog_color, vec3 fragment_color)
{
        float fog_density = 2;

        float fog_start = 0;
        float fog_end = 1;

        float fog_distance = clamp(gl_FragCoord.z - fog_start, 0, fog_end - fog_start);
        float fog_blending_factor = exp(-fog_density * fog_distance);

        return mix(fog_color, fragment_color, fog_blending_factor);
}

void main(void)
{
        vec3 color = mesh.color * drawing.lighting_color * mesh.ambient;

        set_fragment_color(drawing.show_fog ? fog(drawing.background_color, color) : color);
}
