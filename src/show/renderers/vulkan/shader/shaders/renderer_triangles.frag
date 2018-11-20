/*
Copyright (C) 2017, 2018 Topological Manifold

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

// Общие данные для всех треугольников всех объектов
layout(set = 0, binding = 1) uniform Lighting
{
        vec3 direction_to_light;
        vec3 direction_to_camera;
        bool show_smooth;
}
lighting;
layout(set = 0, binding = 2) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        float default_ns;
        vec3 light_a;
        vec3 light_d;
        vec3 light_s;
        bool show_materials;
        bool show_wireframe;
        bool show_shadow;
}
drawing;

layout(set = 0, binding = 3) uniform sampler2D shadow_texture;
layout(set = 0, binding = 4, r32ui) writeonly uniform uimage2D object_image;

// Для каждой группы треугольников с одним материалом отдельно задаётся этот материал и его текстуры
layout(set = 1, binding = 0) uniform Material
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
        vec3 normal;
        vec4 shadow_position;
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

layout(location = 0) out vec4 color;

//

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

void main()
{
        // color = vec4(gs.texture_coordinates, 0, 1);

        vec3 color_a, color_d, color_s;

        if (mtl.use_material && drawing.show_materials)
        {
                if (has_texture_coordinates() && mtl.use_texture_Ka)
                {
                        vec4 tex_color = texture(texture_Ka, gs.texture_coordinates);
                        color_a = mix(mtl.Ka, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_a = mtl.Ka;
                }

                if (has_texture_coordinates() && mtl.use_texture_Kd)
                {
                        vec4 tex_color = texture(texture_Kd, gs.texture_coordinates);
                        color_d = mix(mtl.Kd, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_d = mtl.Kd;
                }

                if (has_texture_coordinates() && mtl.use_texture_Ks)
                {
                        vec4 tex_color = texture(texture_Ks, gs.texture_coordinates);
                        color_s = mix(mtl.Ks, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_s = mtl.Ks;
                }
        }
        else
        {
                color_a = color_d = color_s = drawing.default_color;
        }

        //

        vec3 N = normalize(gs.normal);
        vec3 L = lighting.direction_to_light;
        vec3 V = lighting.direction_to_camera;

        float dot_NL = dot(N, L);
        if (dot_NL >= 0.0)
        {
                color_d *= dot_NL;

                float light_reflection = max(0.0, dot(V, reflect(-L, N)));
                color_s *= pow(light_reflection, (mtl.use_material && drawing.show_materials) ? mtl.Ns : drawing.default_ns);
        }
        else
        {
                color_d = color_s = vec3(0.0);
        }

        //

        vec3 color3;

        if (drawing.show_shadow)
        {
                float dist = texture(shadow_texture, gs.shadow_position.xy).r;
                float shadow = dist > gs.shadow_position.z ? 1 : 0;

                color3 = color_a * drawing.light_a + shadow * (color_d * drawing.light_d + color_s * drawing.light_s);
        }
        else
        {
                color3 = color_a * drawing.light_a + color_d * drawing.light_d + color_s * drawing.light_s;
        }

        if (drawing.show_wireframe)
        {
                color3 = mix(drawing.wireframe_color, color3, edge_factor());
        }

        color = vec4(color3, 1);

        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));
}
