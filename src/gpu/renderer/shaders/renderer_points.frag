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

#version 450

layout(early_fragment_tests) in;

layout(std140, set = 0, binding = 1) uniform Drawing
{
        vec3 default_color;
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
}
drawing;

layout(set = 0, binding = 2, r32ui) writeonly uniform uimage2D object_image;

layout(location = 0) out vec4 color;

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
        vec3 color3;

        if (drawing.show_fog)
        {
                color3 = fog(drawing.background_color, drawing.default_color * drawing.light_a);
        }
        else
        {
                color3 = drawing.default_color * drawing.light_a;
        }

        color = vec4(color3, 1);

        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));
}
