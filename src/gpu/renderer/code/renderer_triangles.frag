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
#include "shading_ggx_diffuse.glsl"

layout(set = 2, binding = 0, std140) uniform Material
{
        vec3 color;
        bool use_texture;
        bool use_material;
}
mtl;
layout(set = 2, binding = 1) uniform sampler2D material_texture;

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

float shadow_weight()
{
        const float d = texture(shadow_texture, gs.shadow_position.xy).r;
        return d <= gs.shadow_position.z ? 1 : 0;
}

bool has_texture_coordinates()
{
        return gs.texture_coordinates[0] > -1e9;
}

float edge_factor()
{
        const vec3 d = 0.5 * fwidth(gs.baricentric);
        const vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

vec3 shade_light(const vec3 color, const vec3 n, const vec3 v)
{
        const vec3 l = drawing.direction_to_light;
        const vec3 s = shading_ggx_diffuse(mesh.metalness, mesh.roughness, color, n, v, l);
        return s;
}

vec3 shade_camera(const vec3 color, const vec3 n, const vec3 v)
{
        const vec3 l = v;
        const vec3 s = shading_ggx_diffuse(mesh.metalness, mesh.roughness, color, n, v, l);
        return s;
}

vec3 shade(const vec3 color)
{
        const vec3 n = normalize(gs.world_normal);
        const vec3 v = drawing.direction_to_camera;

        if (!drawing.show_shadow)
        {
                return shade_light(color, n, v);
        }

        vec3 s = 0.2 * shade_camera(color, n, v);

        const float c = (1 - shadow_weight());
        if (c > 0)
        {
                s += c * 0.8 * shade_light(color, n, v);
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
        else if (!has_texture_coordinates() || !mtl.use_texture)
        {
                color = mtl.color;
        }
        else
        {
                color = texture(material_texture, gs.texture_coordinates).rgb;
        }

        return drawing.lighting_color * shade(color) + mesh.ambient * color;
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
