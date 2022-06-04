/*
Copyright (C) 2017-2022 Topological Manifold

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

#ifdef RAY_TRACING
const float FLOAT_EPSILON = 1.0 / (1 << 23);
const float RAY_OFFSET = 64 * FLOAT_EPSILON;
#endif

layout(location = 0) in GS
{
        vec3 world_normal;
        flat vec3 world_geometric_normal;
        vec3 world_position;
        vec3 baricentric;
        vec2 texture_coordinates;
        flat uint geometric_normal_directed_to_light;
}
gs;

//

#ifdef RAY_TRACING
vec3 ray_org_to_light()
{
        const float ray_offset = gs.geometric_normal_directed_to_light != 0 ? RAY_OFFSET : -RAY_OFFSET;
        vec3 org;
        for (int i = 0; i < 3; ++i)
        {
                org[i] = gs.world_position[i] + (abs(gs.world_position[i]) * ray_offset) * gs.world_geometric_normal[i];
        }
        return org;
}
#else
vec3 ray_org_to_light()
{
        return vec3(0);
}
#endif

bool has_texture_coordinates()
{
        return gs.texture_coordinates[0] > -1e9;
}

vec3 surface_color()
{
        if (!mtl.use_material || !drawing.show_materials)
        {
                return mesh.color;
        }

        if (!has_texture_coordinates() || !mtl.use_texture)
        {
                return mtl.color;
        }

        return texture(material_texture, gs.texture_coordinates).rgb;
}

float edge_factor()
{
        if (!drawing.show_wireframe)
        {
                return 0;
        }

        const vec3 d = 0.5 * fwidth(gs.baricentric);
        const vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return 1 - min(min(a.x, a.y), a.z);
}

void main()
{
        set_fragment_color(
                surface_color(), normalize(gs.world_normal), edge_factor(), gs.geometric_normal_directed_to_light != 0,
                ray_org_to_light());
}
