/*
Copyright (C) 2017-2019 Topological Manifold

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

layout(std140, binding = 1) uniform Lighting
{
        vec3 direction_to_light;
        vec3 direction_to_camera;
        bool show_smooth;
};

in VS
{
        vec2 texture_coordinates;
        vec3 normal;
        vec4 shadow_position;
        flat int material_index;
        flat int property;
        flat vec3 orig_position;
}
vs[3];

out GS
{
        vec2 texture_coordinates;
        vec3 normal;
        vec4 shadow_position;
        flat int material_index;
        flat int property;
        vec3 baricentric;
}
gs;

const vec3 baricentric[3] = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};

vec3[3] compute_normals()
{
        vec3 flat_normal = cross(vs[1].orig_position - vs[0].orig_position, vs[2].orig_position - vs[0].orig_position);

        vec3 normals[3];

        bool face_has_normal = (vs[0].property & 2) > 0;

        if (face_has_normal && show_smooth)
        {
                // Направить векторы вершин грани по направлению перпендикуляра
                // к той стороне грани, которая обращена к камере.

                // direction_to_camera - это направление на камеру.
                // Вектор flat_normal направить от камеры.
                flat_normal = faceforward(flat_normal, direction_to_camera, flat_normal);

                // Повернуть векторы вершин в противоположном вектору flat_normal направлении
                normals[0] = faceforward(vs[0].normal, flat_normal, vs[0].normal);
                normals[1] = faceforward(vs[1].normal, flat_normal, vs[1].normal);
                normals[2] = faceforward(vs[2].normal, flat_normal, vs[2].normal);
        }
        else
        {
                // Направить перпендикуляр к грани на камеру.

                // direction_to_camera - это направление на камеру.
                // Вектор flat_normal направить на камеру.
                flat_normal = faceforward(flat_normal, -direction_to_camera, flat_normal);

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

        for (int i = 0; i < 3; ++i)
        {
                gl_Position = gl_in[i].gl_Position;

                gs.normal = normals[i];
                gs.baricentric = baricentric[i];

                gs.texture_coordinates = vs[i].texture_coordinates;
                gs.shadow_position = vs[i].shadow_position;
                gs.material_index = vs[i].material_index;
                gs.property = vs[i].property;

                EmitVertex();
        }

        EndPrimitive();
}
