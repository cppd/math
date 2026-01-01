/*
Copyright (C) 2017-2026 Topological Manifold

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
        vec3 world_position;
        flat vec3 world_geometric_normal;
        vec3 baricentric;
        vec2 texture_coordinates;
}
gs;

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
                surface_color(), normalize(gs.world_normal), edge_factor(), gs.world_position,
                gs.world_geometric_normal);
}
