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
#include "common.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

//

layout(location = 0) in VS
{
        vec3 world_normal;
        vec3 world_position;
        vec4 shadow_position;
        vec2 texture_coordinates;
}
vs[3];

in gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
}
gl_in[3];

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

layout(location = 0) out GS
{
        vec3 world_normal;
        vec4 shadow_position;
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

//

const vec3 baricentric[3] = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};

vec3[3] compute_normals()
{
        vec3 geometric_world_normal =
                cross(vs[1].world_position - vs[0].world_position, vs[2].world_position - vs[0].world_position);

        vec3 normals[3];

        // Нужно направить векторы вершин грани по направлению перпендикуляра
        // к той стороне грани, которая обращена к камере.
        // direction_to_camera - это направление на камеру.

        if (drawing.show_smooth)
        {
                // в направлении от камеры
                geometric_world_normal =
                        faceforward(geometric_world_normal, drawing.direction_to_camera, geometric_world_normal);

                // Повернуть векторы вершин в противоположном вектору geometric_normal направлении
                normals[0] = faceforward(vs[0].world_normal, geometric_world_normal, vs[0].world_normal);
                normals[1] = faceforward(vs[1].world_normal, geometric_world_normal, vs[1].world_normal);
                normals[2] = faceforward(vs[2].world_normal, geometric_world_normal, vs[2].world_normal);
        }
        else
        {
                // в направлении на камеру
                geometric_world_normal =
                        faceforward(geometric_world_normal, -drawing.direction_to_camera, geometric_world_normal);

                normals[0] = geometric_world_normal;
                normals[1] = geometric_world_normal;
                normals[2] = geometric_world_normal;
        }

        return normals;
}

void main()
{
        vec3 normals[3] = compute_normals();

        for (int i = 0; i < 3; ++i)
        {
                gl_Position = gl_in[i].gl_Position;
                gl_ClipDistance[0] = gl_in[i].gl_ClipDistance[0];

                gs.world_normal = normals[i];
                gs.baricentric = baricentric[i];

                gs.shadow_position = vs[i].shadow_position;
                gs.texture_coordinates = vs[i].texture_coordinates;

                EmitVertex();
        }

        EndPrimitive();
}
