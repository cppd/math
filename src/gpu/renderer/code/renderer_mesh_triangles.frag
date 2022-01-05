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

#extension GL_GOOGLE_include_directive : enable
#include "mesh_common.glsl"
#include "mesh_out.glsl"
#include "shading_ggx_diffuse.glsl"
#include "shading_metalness.glsl"

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

vec3 shade_light(const vec3 f0, const vec3 rho_ss, const vec3 n, const vec3 v)
{
        const vec3 l = drawing.direction_to_light;
        const vec3 s = shading_ggx_diffuse(
                mesh.roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness,
                ggx_f1_albedo_cosine_weighted_average);
        return s;
}

vec3 shade_camera_light(const vec3 f0, const vec3 rho_ss, const vec3 n, const vec3 v)
{
        const vec3 l = v;
        const vec3 s = shading_ggx_diffuse(
                mesh.roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness,
                ggx_f1_albedo_cosine_weighted_average);
        return s;
}

vec3 shade(const vec3 surface_color)
{
        const vec3 n = normalize(gs.world_normal);
        const vec3 v = drawing.direction_to_camera;

        const vec3 f0 = shading_compute_metalness_f0(surface_color, mesh.metalness);
        const vec3 rho_ss = shading_compute_metalness_rho_ss(surface_color, mesh.metalness);

        if (!drawing.show_shadow)
        {
                return shade_light(f0, rho_ss, n, v);
        }

        vec3 s = 0.2 * shade_camera_light(f0, rho_ss, n, v);

        const float c = (1 - shadow_weight());
        if (c > 0)
        {
                s += c * 0.8 * shade_light(f0, rho_ss, n, v);
        }

        return s;
}

vec3 shade()
{
        vec3 surface_color;

        if (!mtl.use_material || !drawing.show_materials)
        {
                surface_color = mesh.color;
        }
        else if (!has_texture_coordinates() || !mtl.use_texture)
        {
                surface_color = mtl.color;
        }
        else
        {
                surface_color = texture(material_texture, gs.texture_coordinates).rgb;
        }

        return drawing.lighting_color * shade(surface_color) + mesh.ambient * surface_color;
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
