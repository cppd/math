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
        vec3 d = 0.5 * fwidth(gs.baricentric);
        vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

vec3 shade_light(vec3 color, vec3 n, vec3 v)
{
        vec3 l = drawing.direction_to_light;
        vec3 s = shade(mesh.metalness, mesh.roughness, color, n, v, l);
        return s;
}

vec3 shade_camera(vec3 color, vec3 n, vec3 v)
{
        vec3 l = v;
        vec3 s = shade(mesh.metalness, mesh.roughness, color, n, v, l);
        return s;
}

vec3 shade(vec3 color)
{
        vec3 n = normalize(gs.world_normal);
        vec3 v = drawing.direction_to_camera;

        if (!drawing.show_shadow)
        {
                return shade_light(color, n, v);
        }

        vec3 s = 0.2 * shade_camera(color, n, v);
        if (!shadow(gs.shadow_position))
        {
                s += 0.8 * shade_light(color, n, v);
        }
        return s;
}

vec3 shade()
{
        vec3 color;

        if (!mtl.use_material || !drawing.show_materials)
        {
                color = mesh.color;
        }
        else if (!has_texture_coordinates() || !mtl.use_texture_Kd)
        {
                color = mtl.Kd;
        }
        else
        {
                color = texture_Kd_color(gs.texture_coordinates).rgb;
        }

        return drawing.lighting_intensity * shade(color) + mesh.ambient * color;
}

void main()
{
        vec3 color = shade();

        if (drawing.show_wireframe)
        {
                color = mix(drawing.wireframe_color, color, edge_factor());
        }

        set_fragment_color(color);
}
