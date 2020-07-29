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

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(early_fragment_tests) in;

// Для каждой группы треугольников с одним материалом отдельно задаётся этот материал и его текстуры
layout(set = 2, binding = 0, std140) uniform Material
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
layout(set = 2, binding = 1) uniform sampler2D texture_Ka;
layout(set = 2, binding = 2) uniform sampler2D texture_Kd;
layout(set = 2, binding = 3) uniform sampler2D texture_Ks;

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
bool shadow(vec4 shadow_position)
{
        float dist = texture(shadow_texture, shadow_position.xy).r;
        return dist <= gs.shadow_position.z;
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

float material_ns()
{
        return (mtl.use_material && drawing.show_materials) ? mtl.Ns : drawing.default_ns;
}

vec3 shade()
{
        //

        vec3 mtl_a, mtl_d, mtl_s;

        if (!mtl.use_material || !drawing.show_materials)
        {
                mtl_a = drawing.default_color;
                mtl_d = drawing.default_color;
                mtl_s = drawing.default_specular_color;
        }
        else if (!has_texture_coordinates())
        {
                mtl_a = mtl.Ka;
                mtl_d = mtl.Kd;
                mtl_s = mtl.Ks;
        }
        else
        {
                mtl_a = mtl.use_texture_Ka ? texture_Ka_color(gs.texture_coordinates).rgb : mtl.Ka;
                mtl_d = mtl.use_texture_Kd ? texture_Kd_color(gs.texture_coordinates).rgb : mtl.Kd;
                mtl_s = mtl.use_texture_Ks ? texture_Ks_color(gs.texture_coordinates).rgb : mtl.Ks;
        }

        vec3 color = mtl_a * drawing.light_a;

        if (!drawing.show_shadow || !shadow(gs.shadow_position))
        {
                vec3 N = normalize(gs.world_normal);
                vec3 L = drawing.direction_to_light;
                vec3 V = drawing.direction_to_camera;

                float diffuse = max(0, dot(N, L));
                if (diffuse > 0)
                {
                        float specular = pow(max(0, dot(V, reflect(-L, N))), material_ns());
                        color += diffuse * mtl_d * drawing.light_d;
                        color += specular * mtl_s * drawing.light_s;
                }
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
