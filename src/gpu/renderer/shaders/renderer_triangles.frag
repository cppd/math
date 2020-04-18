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

// Общие данные для всех треугольников всех объектов
layout(std140, binding = 1) uniform Lighting
{
        vec3 direction_to_light;
        vec3 direction_to_camera;
        bool show_smooth;
}
lighting;
layout(std140, binding = 2) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        vec3 background_color;
        vec3 clip_plane_color;
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
        vec4 clip_plane_equation;
        bool clip_plane_enabled;
}
drawing;

layout(binding = 3) uniform sampler2D shadow_texture;
layout(binding = 4, r32ui) writeonly uniform uimage2D object_image;

// Для каждой группы треугольников с одним материалом отдельно задаётся этот материал и его текстуры
layout(std140, set = 1, binding = 0) uniform Material
{
        vec3 Ka;
        vec3 Kd;
        vec3 Ks;
        float Ns;
        bool use_texture_Ka;
        bool use_texture_Kd;
        bool use_texture_Ks;
        bool use_material;
}
mtl;
layout(set = 1, binding = 1) uniform sampler2D texture_Ka;
layout(set = 1, binding = 2) uniform sampler2D texture_Kd;
layout(set = 1, binding = 3) uniform sampler2D texture_Ks;

//

layout(location = 0) in GS
{
        vec3 world_normal;
        vec4 shadow_position;
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

layout(location = 0) out vec4 color;

//

vec4 texture_Ka_color(vec2 c)
{
        return texture(texture_Ka, c);
}
vec4 texture_Kd_color(vec2 c)
{
        return texture(texture_Kd, c);
}
vec4 texture_Ks_color(vec2 c)
{
        return texture(texture_Ks, c);
}
float shadow(vec4 shadow_position)
{
        float dist = texture(shadow_texture, shadow_position.xy).r;
        return dist > gs.shadow_position.z ? 1 : 0;
}

bool has_texture_coordinates()
{
        // Если нет текстурных координат, то они задаются числом -1e10
        return gs.texture_coordinates[0] > -1e9;
}

float edge_factor()
{
        vec3 d = fwidth(gs.baricentric);
        vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

vec3 shade()
{
        vec3 color_a, color_d, color_s;

        if (!mtl.use_material || !drawing.show_materials)
        {
                color_a = drawing.default_color;
                color_d = drawing.default_color;
                color_s = drawing.default_color;
        }
        else if (!has_texture_coordinates())
        {
                color_a = mtl.Ka;
                color_d = mtl.Kd;
                color_s = mtl.Ks;
        }
        else
        {
                color_a = mtl.use_texture_Ka ? texture_Ka_color(gs.texture_coordinates).rgb : mtl.Ka;
                color_d = mtl.use_texture_Kd ? texture_Kd_color(gs.texture_coordinates).rgb : mtl.Kd;
                color_s = mtl.use_texture_Ks ? texture_Ks_color(gs.texture_coordinates).rgb : mtl.Ks;
        }

        //

        vec3 N = normalize(gs.world_normal);
        vec3 L = lighting.direction_to_light;
        vec3 V = lighting.direction_to_camera;

        float dot_NL = dot(N, L);
        if (dot_NL >= 0.0)
        {
                color_d *= dot_NL;

                float light_reflection = max(0.0, dot(V, reflect(-L, N)));
                color_s *= pow(
                        light_reflection, (mtl.use_material && drawing.show_materials) ? mtl.Ns : drawing.default_ns);
        }
        else
        {
                color_d = vec3(0.0);
                color_s = vec3(0.0);
        }

        //

        vec3 color;

        if (drawing.show_shadow)
        {
                const float s = shadow(gs.shadow_position);
                color = color_a * drawing.light_a + s * (color_d * drawing.light_d + color_s * drawing.light_s);
        }
        else
        {
                color = color_a * drawing.light_a + color_d * drawing.light_d + color_s * drawing.light_s;
        }

        if (drawing.show_wireframe)
        {
                color = mix(drawing.wireframe_color, color, edge_factor());
        }

        return color;
}

void main()
{
        color = vec4(shade(), 1);

        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));
}
