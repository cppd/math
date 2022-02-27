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

layout(location = 0) in GS
{
        vec3 world_normal;
#ifdef RAY_TRACING
        vec3 world_position;
#else
        vec3 shadow_position;
#endif
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

//

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
                return -1;
        }
        const vec3 d = 0.5 * fwidth(gs.baricentric);
        const vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

vec3 normal()
{
        return normalize(gs.world_normal);
}

#ifdef RAY_TRACING
vec3 position_for_shadow()
{
        return gs.world_position;
}
#else
vec3 position_for_shadow()
{
        return gs.shadow_position;
}
#endif

void main()
{
        set_color(surface_color(), normal(), position_for_shadow(), edge_factor());
}
