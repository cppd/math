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
#include "shading.glsl"

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

vec3 shade(vec3 color)
{
        vec3 n = normalize(gs.world_normal);
        vec3 v = drawing.direction_to_camera;
        vec3 l = drawing.direction_to_light;

        return shade(drawing.lighting_intensity, mesh.metalness, mesh.roughness, color, n, v, l);
}

vec3 shade()
{
        vec3 mtl_color;

        if (!mtl.use_material || !drawing.show_materials)
        {
                mtl_color = mesh.color;
        }
        else if (!has_texture_coordinates() || !mtl.use_texture_Kd)
        {
                mtl_color = mtl.Kd;
        }
        else
        {
                mtl_color = texture_Kd_color(gs.texture_coordinates).rgb;
        }

        vec3 color = mesh.ambient * mtl_color;
        if (!drawing.show_shadow)
        {
                color += shade(mtl_color);
        }
        else
        {
                bool s = shadow(gs.shadow_position);
                if (!s)
                {
                        color += shade(mtl_color);
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
        set_fragment_color(shade());
}
