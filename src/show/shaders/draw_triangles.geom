/*
Copyright (C) 2017 Topological Manifold

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

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform int show_smooth;

in vec2 vs_tex_coord[];
in vec3 vs_normal[];
in vec4 vs_shadow_coord[];
flat in int vs_material_index[];
flat in int vs_property[];
flat in vec3 orig_position[];

out vec4 gs_shadow_coord;
out vec2 gs_tex_coord;
out vec3 gs_normal;
out vec3 gs_baricentric;
flat out int gs_material_index;
flat out int gs_property;

void main(void)
{
        vec3 normal[3];

        // vs_property[0] & 2 > 0 - есть нормаль
        if ((vs_property[0] & 2) > 0 && show_smooth > 0)
        {
                normal[0] = vs_normal[0];
                normal[1] = vs_normal[1];
                normal[2] = vs_normal[2];
        }
        else
        {
                vec3 n = cross(orig_position[1] - orig_position[0], orig_position[2] - orig_position[0]);
                normal[0] = n;
                normal[1] = n;
                normal[2] = n;
        }

        gl_Position = gl_in[0].gl_Position;
        gs_tex_coord = vs_tex_coord[0];
        gs_shadow_coord = vs_shadow_coord[0];
        gs_normal = normal[0];
        gs_baricentric = vec3(1, 0, 0);
        gs_material_index = vs_material_index[0];
        gs_property = vs_property[0];
        EmitVertex();

        gl_Position = gl_in[1].gl_Position;
        gs_tex_coord = vs_tex_coord[1];
        gs_shadow_coord = vs_shadow_coord[1];
        gs_normal = normal[1];
        gs_baricentric = vec3(0, 1, 0);
        gs_material_index = vs_material_index[1];
        gs_property = vs_property[1];
        EmitVertex();

        gl_Position = gl_in[2].gl_Position;
        gs_tex_coord = vs_tex_coord[2];
        gs_shadow_coord = vs_shadow_coord[2];
        gs_normal = normal[2];
        gs_baricentric = vec3(0, 0, 1);
        gs_material_index = vs_material_index[2];
        gs_property = vs_property[2];
        EmitVertex();

        EndPrimitive();
}
