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

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "mesh_in.glsl"
#include "mesh_out.glsl"

vec3 fog(const vec3 fog_color, const vec3 color)
{
        const float fog_density = 2;

        const float fog_start = 0;
        const float fog_end = 1;

        const float fog_distance = clamp(gl_FragCoord.z - fog_start, 0, fog_end - fog_start);
        const float fog_blending_factor = exp(-fog_density * fog_distance);

        return mix(fog_color, color, fog_blending_factor);
}

void main()
{
        const vec3 color = mesh.color * drawing.lighting_color * mesh.ambient;

        set_fragment_color(drawing.show_fog ? fog(drawing.background_color, color) : color);
}
