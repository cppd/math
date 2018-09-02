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
uniform vec3 camera_direction;

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

vec3[3] compute_normals()
{
        vec3 flat_normal = cross(orig_position[1] - orig_position[0], orig_position[2] - orig_position[0]);

        vec3 normals[3];

        bool face_has_normal = (vs_property[0] & 2) > 0;

        if (face_has_normal && show_smooth > 0)
        {
                // Направить векторы вершин грани по направлению перпендикуляра
                // к той стороне грани, которая обращена к камере.

                // camera_direction - это направление на камеру.
                // Вектор flat_normal направить от камеры.
                flat_normal = faceforward(flat_normal, camera_direction, flat_normal);

                // Повернуть векторы вершин в противоположном вектору flat_normal направлении
                normals[0] = faceforward(vs_normal[0], flat_normal, vs_normal[0]);
                normals[1] = faceforward(vs_normal[1], flat_normal, vs_normal[1]);
                normals[2] = faceforward(vs_normal[2], flat_normal, vs_normal[2]);
        }
        else
        {
                // Направить перпендикуляр к грани на камеру.

                // camera_direction - это направление на камеру.
                // Вектор flat_normal направить на камеру.
                flat_normal = faceforward(flat_normal, -camera_direction, flat_normal);

                // Задать векторы вершин вектором flat_normal
                normals[0] = flat_normal;
                normals[1] = flat_normal;
                normals[2] = flat_normal;
        }

        return normals;
}

void main(void)
{
        vec3 normals[3] = compute_normals();

        gl_Position = gl_in[0].gl_Position;
        gs_tex_coord = vs_tex_coord[0];
        gs_shadow_coord = vs_shadow_coord[0];
        gs_normal = normals[0];
        gs_baricentric = vec3(1, 0, 0);
        gs_material_index = vs_material_index[0];
        gs_property = vs_property[0];
        EmitVertex();

        gl_Position = gl_in[1].gl_Position;
        gs_tex_coord = vs_tex_coord[1];
        gs_shadow_coord = vs_shadow_coord[1];
        gs_normal = normals[1];
        gs_baricentric = vec3(0, 1, 0);
        gs_material_index = vs_material_index[1];
        gs_property = vs_property[1];
        EmitVertex();

        gl_Position = gl_in[2].gl_Position;
        gs_tex_coord = vs_tex_coord[2];
        gs_shadow_coord = vs_shadow_coord[2];
        gs_normal = normals[2];
        gs_baricentric = vec3(0, 0, 1);
        gs_material_index = vs_material_index[2];
        gs_property = vs_property[2];
        EmitVertex();

        EndPrimitive();
}
