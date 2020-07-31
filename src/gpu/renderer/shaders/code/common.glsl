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

layout(set = 0, binding = 0, std140) uniform Matrices
{
        mat4 vp_matrix;
        mat4 shadow_vp_texture_matrix;
}
matrices;

layout(set = 0, binding = 1, std140) uniform Drawing
{
        vec3 default_color;
        vec3 default_specular_color;
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

layout(set = 0, binding = 2) uniform sampler2D shadow_texture;
layout(set = 0, binding = 3, r32ui) uniform writeonly uimage2D object_image;

//

layout(set = 1, binding = 0, std140) uniform Mesh
{
        mat4 model_matrix;
        mat3 normal_matrix;
        float alpha;
}
mesh;

//

#if defined(FRAGMENT_SHADER)

layout(early_fragment_tests) in;

layout(location = 0) out vec4 out_color;

void set_fragment_color(vec3 color)
{
        out_color = vec4(color, 1);
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));
}

#endif
